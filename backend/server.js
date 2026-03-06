const express = require('express');
const multer = require('multer');
const { exec } = require('child_process');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

const app = express();
const port = 3001;

// 配置CORS
app.use(cors());

// 配置文件上传
const storage = multer.diskStorage({
  destination: function (req, file, cb) {
    cb(null, 'uploads/') // 确保这个目录存在
  },
  filename: function (req, file, cb) {
    cb(null, Date.now() + '-' + file.originalname)
  }
});

const upload = multer({ storage: storage });

// 静态文件服务
app.use('/output', express.static('output'));

// 处理文件上传和C++程序调用
app.post('/process', upload.single('verilogFile'), (req, res) => {
  if (!req.file) {
    return res.status(400).json({ error: '没有上传文件' });
  }

  const inputFile = req.file.path;
  // 假设您的C++可执行文件在上级目录的Main/x64/Debug/下
  const cppExecutablePath = path.join(__dirname, './Main.exe');
  
  // 构建命令
  const command = `"${cppExecutablePath}" "${inputFile}"`;

  console.log('执行命令:', command);

  exec(command, (error, stdout, stderr) => {
    if (error) {
      console.error(`执行错误: ${error}`);
      return res.status(500).json({ 
        error: '处理文件时发生错误',
        details: error.message
      });
    }

    // 检查是否生成了图片
    const outputImagePath = path.join(__dirname, '../output/show.dot.png');
    const imageUrl = fs.existsSync(outputImagePath) 
      ? `http://localhost:${port}/output/show.dot.png` 
      : null;

    res.json({
      result: stdout,
      stderr: stderr,
      imageUrl: imageUrl
    });

    // 清理上传的文件
    fs.unlink(inputFile, (err) => {
      if (err) console.error('清理文件失败:', err);
    });
  });
});

// 错误处理中间件
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({ 
    error: '服务器内部错误',
    details: err.message 
  });
});

// 启动服务器
app.listen(port, () => {
  console.log(`服务器运行在 http://localhost:${port}`);
}); 