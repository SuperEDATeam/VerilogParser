import React, { useState } from 'react';
import './App.css';

interface ProcessResponse {
  result: string;
  stderr?: string;
  imageUrl?: string;
  error?: string;
  details?: string;
}

function App() {
  const [file, setFile] = useState<File | null>(null);
  const [result, setResult] = useState<string>('');
  const [error, setError] = useState<string>('');
  const [imageUrl, setImageUrl] = useState<string>('');
  const [loading, setLoading] = useState<boolean>(false);

  const handleFileChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (event.target.files && event.target.files[0]) {
      setFile(event.target.files[0]);
      setError('');
    }
  };

  const handleSubmit = async (event: React.FormEvent) => {
    event.preventDefault();
    if (!file) return;

    setLoading(true);
    setError('');
    setResult('');
    setImageUrl('');

    const formData = new FormData();
    formData.append('verilogFile', file);

    try {
      const response = await fetch('http://localhost:3001/process', {
        method: 'POST',
        body: formData,
      });
      
      const data: ProcessResponse = await response.json();
      
      if (data.error) {
        setError(`错误: ${data.error}${data.details ? `\n详细信息: ${data.details}` : ''}`);
        return;
      }

      setResult(data.result);
      if (data.stderr) {
        setError(data.stderr);
      }
      if (data.imageUrl) {
        setImageUrl(data.imageUrl);
      }
    } catch (error) {
      setError(`处理出错: ${error instanceof Error ? error.message : String(error)}`);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="container">
      <h1>Verilog文件处理系统</h1>
      
      <form onSubmit={handleSubmit} className="upload-form">
        <div className="upload-section">
          <input
            type="file"
            accept=".v,.verilog"
            onChange={handleFileChange}
            className="file-input"
          />
          <button 
            type="submit" 
            disabled={!file || loading}
            className="submit-button"
          >
            {loading ? '处理中...' : '开始处理'}
          </button>
        </div>
      </form>

      {error && (
        <div className="error-section">
          <h2>错误信息：</h2>
          <pre>{error}</pre>
        </div>
      )}

      {result && (
        <div className="result-section">
          <h2>处理结果：</h2>
          <pre>{result}</pre>
        </div>
      )}

      {imageUrl && (
        <div className="image-section">
          <h2>生成的图片：</h2>
          <div className="image-container">
            <img 
              src={imageUrl} 
              alt="生成的图" 
              className="result-image"
              onError={(e) => {
                setError('图片加载失败');
                e.currentTarget.style.display = 'none';
              }}
            />
            <div className="image-controls">
              <button 
                onClick={() => window.open(imageUrl, '_blank')}
                className="view-button"
              >
                在新窗口查看
              </button>
              <a 
                href={imageUrl} 
                download="verilog-diagram.png"
                className="download-button"
              >
                下载图片
              </a>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default App; 