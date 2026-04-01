#include <stdexcept> 
#include "MyMidData.h"
# include "utils.h"


void PAssign::parse(MyMidModule* pmod) {
    std::string op;
    string lstr, rstr, rrstr;
    if (splitByOperator(m_whole_exp, lstr, rstr, op)) {

        PEIdent* l_val = new PEIdent;
        l_val->name = lstr;
        pins_.push_back(l_val);

        OPType opt = judgeOperation(m_whole_exp);//目前可判断的表达式有二元表达式和比较表达式
        switch (opt) {
        case TBinary: {
            PEBinary* r_val = new PEBinary;
            r_val->parse(pmod, rstr);
            pins_.push_back(r_val);
            break;
        }
        case TComparison: {
            PEComparison* r_val = new PEComparison;
            r_val->parse(pmod, rstr);
            pins_.push_back(r_val);
            break;
        }
        case TNULL: {
            PEUnary* r_val = new PEUnary;
            r_val->parse(pmod, rstr);
            pins_.push_back(r_val);
            break;
        }
        }
    }
}

void PEBinary::parse(MyMidModule* pmod, string exp) {
    std::string op,test_op;
    string lstr, rstr,test_lstr,test_rstr;

    if (splitByOperator(exp, lstr, rstr, op)) {
        op_ = op;
        if (lstr.size() > 0)
        {
            if (!pmod->is_in_wires(lstr) && !splitByOperator(lstr, test_lstr, test_rstr, test_op)) {
                throw runtime_error("undefined variable: " + lstr);
            }
            if (!test_op.empty()) {
                OPType opt = judgeOperation(lstr);
                switch (opt) {
                case TBinary:
                    left_ = new PEBinary;
                    left_->parse(pmod, lstr);
                    break;
                case TComparison:
                    left_ = new PEComparison;
                    left_->parse(pmod, lstr);
                    break;
                default:
                    left_ = new PEIdent;
                    left_->parse(pmod, lstr);
                    break;
                }
            }
            else {
                left_ = new PEIdent;
                left_->parse(pmod, lstr);
            }
            //pins_.push_back(l_val);
            }
        if (rstr.size() > 0)
        {
            OPType opt = judgeOperation(rstr);
            switch (opt) {
            case TNULL:
                if (!pmod->is_in_wires(rstr))
                    throw runtime_error("undefine variable" + rstr);
                right_ = new PEIdent;
                right_->parse(pmod, rstr);
                break;
                //case TUnary:

            case TBinary:
                right_ = new PEBinary;
                right_->parse(pmod, rstr);
                //pins_.push_back(r_val);
            case TComparison:
                right_ = new PEComparison;
                right_->parse(pmod, rstr);
            }

        }
    }
}


void PEIdent::parse(MyMidModule* pmod, string exp){
    name = exp;
}

void PEComparison::parse(MyMidModule* pmod, string exp)
{
    std::string op;
    string lstr, rstr;

    // 检查表达式是否为空
    if (exp.empty()) {
        throw std::runtime_error("Empty expression passed to PEComparison");
    }

    // 分割表达式
    bool result = splitByOperator(exp, lstr, rstr, op);
    if (!result || lstr.empty() || rstr.empty()) {
        throw std::runtime_error("Operator split failed or invalid expression format");
    }

    op_ = op;
    // 检查左边部分
    if (lstr.size() > 0)
    {
        if (!pmod->is_in_wires(lstr))
            throw runtime_error("undefined variable: " + lstr);
        left_ = new PEIdent;
        left_->parse(pmod, lstr);
        //pins_.push_back(l_val);  // 如果需要添加到pins中，可解除注释
    }

    // 检查右边部分
    if (rstr.size() > 0)
    {
        OPType opt = judgeOperation(rstr);
        switch (opt) {
        case TNULL:
            if (!pmod->is_in_wires(rstr))
                throw runtime_error("undefined variable: " + rstr);
            right_ = new PEIdent;
            right_->parse(pmod, rstr);
            break;

        case TUnary:
            right_ = new PEUnary;
            right_->parse(pmod, rstr);
            break;

        case TBinary:
            right_ = new PEBinary;
            right_->parse(pmod, rstr);
            break;

        case TComparison:
            // 防止递归深度过大，添加递归深度检查
            if (rstr.length() > 1000) { // 可根据需要调整深度限制
                throw std::runtime_error("Expression too complex, recursion depth exceeded");
            }
            right_ = new PEComparison;
            static_cast<PEComparison*>(right_)->parse(pmod, rstr);  // 递归解析
            break;

        default:
            throw std::runtime_error("Unsupported operation type in right operand");
        }
    }
}


void PEUnary::parse(MyMidModule* pmod, string exp)
{
    if (exp.empty()) {
        throw std::runtime_error("Empty expression passed to PEComparison");
    }
    op_ = NULL;
    expr_ = new PEIdent;
    expr_->parse(pmod, exp);
}
