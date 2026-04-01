import React, { useState } from 'react';
import './App.css';

interface ProcessResponse {
  result:        string;
  stderr?:       string;
  imageUrl?:     string;
  rtlilContent?: string;
  rtlilUrl?:     string;
  error?:        string;
  details?:      string;
}

function App() {
  const [file,         setFile]         = useState<File | null>(null);
  const [result,       setResult]       = useState<string>('');
  const [error,        setError]        = useState<string>('');
  const [imageUrl,     setImageUrl]     = useState<string>('');
  const [rtlilContent, setRtlilContent] = useState<string>('');
  const [rtlilUrl,     setRtlilUrl]     = useState<string>('');
  const [loading,      setLoading]      = useState<boolean>(false);

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
    setRtlilContent('');
    setRtlilUrl('');

    const formData = new FormData();
    formData.append('verilogFile', file);

    try {
      const response = await fetch('http://localhost:3001/process', {
        method: 'POST',
        body: formData,
      });
      const data: ProcessResponse = await response.json();

      if (data.error) {
        setError(`Error: ${data.error}${data.details ? `\n${data.details}` : ''}`);
        return;
      }

      setResult(data.result ?? '');
      if (data.stderr)       setError(data.stderr);
      if (data.imageUrl)     setImageUrl(data.imageUrl);
      if (data.rtlilContent) setRtlilContent(data.rtlilContent);
      if (data.rtlilUrl)     setRtlilUrl(data.rtlilUrl);

    } catch (err) {
      setError(`Request failed: ${err instanceof Error ? err.message : String(err)}`);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="container">
      <h1>VerilogCompiler Web UI</h1>
      <p className="subtitle">Upload a <code>.v</code> file to compile, visualise and generate RTLIL.</p>

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
            {loading ? 'Processing...' : 'Run'}
          </button>
        </div>
        {file && <p className="file-name">Selected: {file.name}</p>}
      </form>

      {error && (
        <div className="error-section">
          <h2>Warnings / Errors</h2>
          <pre>{error}</pre>
        </div>
      )}

      {result && (
        <div className="result-section">
          <h2>Compiler Output</h2>
          <pre>{result}</pre>
        </div>
      )}

      {imageUrl && (
        <div className="image-section">
          <h2>Circuit Diagram</h2>
          <div className="image-container">
            <img
              src={imageUrl}
              alt="Circuit diagram"
              className="result-image"
              onError={(e) => {
                setError('Image failed to load (Graphviz may not be installed).');
                e.currentTarget.style.display = 'none';
              }}
            />
            <div className="image-controls">
              <button onClick={() => window.open(imageUrl, '_blank')} className="view-button">
                Open in new tab
              </button>
              <a href={imageUrl} download="circuit.png" className="download-button">
                Download PNG
              </a>
            </div>
          </div>
        </div>
      )}

      {rtlilContent && (
        <div className="result-section">
          <h2>
            RTLIL Output
            {rtlilUrl && (
              <a href={rtlilUrl} download="output.il" className="download-button" style={{ marginLeft: '1rem', fontSize: '0.8rem' }}>
                Download .il
              </a>
            )}
          </h2>
          <pre className="rtlil-block">{rtlilContent}</pre>
        </div>
      )}
    </div>
  );
}

export default App;
