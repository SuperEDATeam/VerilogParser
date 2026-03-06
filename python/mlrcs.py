import sys
class Node:

    def __init__(self, name, duration, scheduled=False, starttime=-1, pred=None, end=False,gates=None):
        self.name = name
        self.duration = duration  #持续时间
        self.scheduled = scheduled #是否是已调度
        self.starttime = starttime  #开始时间，起始节点置为0，其余-1
        self.pred = pred if pred else []  #前置节点
        self.end = end  #是否是结束节点
        self.alap_starttime=9999  #ALAP算法得到的最晚开始时间
        self.gates=gates

def parse_gate_parameters():
    if len(sys.argv) != 7:
        print("用法: python your_script.py <and_duration> <or_duration> <not_duration> <and_limit> <or_limit> <not_limit>")
        sys.exit(1)

    try:
        and_duration = int(sys.argv[1])
        or_duration = int(sys.argv[2])
        not_duration = int(sys.argv[3])
        and_limit = int(sys.argv[4])
        or_limit = int(sys.argv[5])
        not_limit = int(sys.argv[6])
    except ValueError:
        print("错误: 所有参数都必须是整数。")
        sys.exit(1)

    if min(and_duration, or_duration, not_duration, and_limit, or_limit, not_limit) < 0:
        print("错误: 所有参数必须是非负整数。")
        sys.exit(1)

    return and_duration, or_duration, not_duration, and_limit, or_limit, not_limit

def read_blif(inputfile,and_duration,or_duration,not_duration):  #读取blif文件，创建nodes[],存放各初始化的node节点
    with open(inputfile, "r") as f:
        lines = f.readlines()

    nodes = []
    start_nodes = []
    end_nodes = []
    all_outputs = set()
    node_map = {}

    for i, line in enumerate(lines):
            if line.startswith(".inputs"):
                inputs = line.strip().split()[1:]
                for name in inputs:
                    node = Node(name, 1, True, 0)
                    nodes.append(node)
                    node_map[name] = node
                    start_nodes.append(node)
            elif line.startswith(".outputs"):
                outputs = line.strip().split()[1:]
                all_outputs.update(outputs)
            elif line.startswith(".names"):
                parts = line.strip().split()
                preds = parts[1:-1]
                name = parts[-1]
                if name in node_map:
                    # 已存在该节点，合并 pred（不重复）
                    existing_node = node_map[name]
                    existing_node.pred = list(set(existing_node.pred + preds))
                else:
                    if (i+2 <len(lines)) and (not lines[i+1].startswith("0")) and (not lines[i+2].startswith(".names")) and (not lines[i+2].startswith(".end")):
                            node = Node(name, or_duration, False, -1, preds, name in all_outputs,"or")
                    elif((i+1 <len(lines)) and (lines[i+1].startswith("0"))):
                        node = Node(name, not_duration, False, -1, preds, name in all_outputs,"not")
                    else:
                        node = Node(name, and_duration, False, -1, preds, name in all_outputs,"and")
                    nodes.append(node)
                    node_map[name] = node
                    if node.end:
                        end_nodes.append(node)
    
    return nodes, end_nodes, node_map,start_nodes

def find_next_node(nodes):  #查询下一个可行节点
    for node in nodes:
        if not node.scheduled:
            all_pred_scheduled = True
            pred_nodes = []
            for pred_name in node.pred:
                pred_node = next((n for n in nodes if n.name == pred_name), None)
                if pred_node:
                    pred_nodes.append(pred_node)
                    if not pred_node.scheduled:
                        all_pred_scheduled = False
                        break
            if all_pred_scheduled:
                return node, pred_nodes
    return None, []

def schedule_node(node, pred_nodes):
    pred_finish_time = [n.starttime + n.duration for n in pred_nodes]
    node.starttime = max(pred_finish_time) if pred_finish_time else 0
    node.scheduled = True

def ml_rcs_schedule(nodes, node_map, and_limit, or_limit, not_limit):
    resource_limits = {'and': and_limit, 'or': or_limit, 'not': not_limit}
    time_step = 1

    while True:
        scheduled_any = False

        for gate_type in ['and', 'or', 'not']:
            limit = resource_limits[gate_type]

            # 找到所有就绪的节点（该类型 + 所有前驱都已调度）
            ready = []
            for node in nodes:
                if not node.scheduled and node.gates == gate_type:
                    preds_ready = all(node_map[p].scheduled for p in node.pred)
                    if preds_ready:
                        ready.append(node)

            # 当前正在执行的节点（未结束）
            ongoing = [
                n for n in nodes if n.scheduled and
                (n.starttime + n.duration > time_step) and n.gates == gate_type
            ]

            available_slots = max(0, limit - len(ongoing))
            schedulable = ready[:available_slots]

            for node in schedulable:
                pred_nodes = [node_map[p] for p in node.pred]
                node.starttime = max([p.starttime + p.duration for p in pred_nodes] or [0])
                if node.starttime < time_step:
                    node.starttime = time_step
                node.scheduled = True
                scheduled_any = True

                # 输出调度信息
                print(f"[调度] 门名称: {node.name}, 类型: {node.gates.upper()}, 起始时间: Cycle {node.starttime}")

        if not scheduled_any:
            if all(n.scheduled for n in nodes if not n.end):
                break

        time_step += 1

if __name__ == "__main__":
    # 主调度逻辑
    inputfile="..\\output\\output.blif"

    and_duration, or_duration, not_duration, and_limit, or_limit, not_limit = parse_gate_parameters()
    nodes, endnodes, node_map,start_nodes = read_blif(inputfile,and_duration,or_duration,not_duration)

    # print("已指定耗时。")

    # while True:
    #     if all(node.scheduled for node in endnodes):
    #         break
    #     node_to_schedule, pred_nodes = find_next_node(nodes)
    #     if node_to_schedule:
    #         schedule_node(node_to_schedule, pred_nodes)
    #     else:
    #         break  # 没有可以调度的节点，可能存在循环依赖

    # # 清空已有调度信息（为了重新调度）
    # for node in nodes:
    #     if node.name not in [n.name for n in start_nodes]:
    #         node.scheduled = False
    #         node.starttime = -1

    # 进行 ML-RCS 调度（资源限制：比如每个周期最多2个AND，1个OR，1个NOT）

    ml_rcs_schedule(nodes, node_map, and_limit, or_limit, not_limit)
