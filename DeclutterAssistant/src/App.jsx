import { useState, useEffect } from 'react';
import './App.css';

function App() {
  const [files, setFiles] = useState([]);
  const [directory, setDirectory] = useState('C:\\');
  const [totalSize, setTotalSize] = useState(0);
  const [newFileName, setNewFileName] = useState('');

  const API_URL = 'http://localhost:8080';

  const fetchFiles = async () => {
    try {
      const response = await fetch(`${API_URL}/files?directory=${encodeURIComponent(directory)}`);
      const data = await response.json();
      setFiles(data.files || []);
      
      const total = data.files?.reduce((sum, file) => sum + file.size, 0) || 0;
      setTotalSize(total);
    } catch (error) {
      console.error('Error fetching files:', error);
      alert('Failed to connect to backend. Make sure the C++ server is running on port 8080.');
    }
  };

  const deleteFile = async (filepath) => {
    if (!window.confirm(`Are you sure you want to delete ${filepath}?`)) return;
    
    try {
      const response = await fetch(`${API_URL}/file`, {
        method: 'DELETE',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ filepath })
      });
      const data = await response.json();
      
      if (data.success) {
        alert('File deleted successfully!');
        fetchFiles();
      } else {
        alert(data.message || 'Failed to delete file');
      }
    } catch (error) {
      console.error('Error deleting file:', error);
      alert('Failed to delete file');
    }
  };

  const createFile = async () => {
    if (!newFileName) {
      alert('Please enter a filename');
      return;
    }
    
    try {
      const response = await fetch(`${API_URL}/file`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ 
          directory: directory,
          filename: newFileName 
        })
      });
      const data = await response.json();
      
      if (data.success) {
        alert('File created successfully!');
        setNewFileName('');
        fetchFiles();
      } else {
        alert(data.message || 'Failed to create file');
      }
    } catch (error) {
      console.error('Error creating file:', error);
      alert('Failed to create file');
    }
  };

  const formatSize = (bytes) => {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
    if (bytes < 1024 * 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
    return (bytes / (1024 * 1024 * 1024)).toFixed(2) + ' GB';
  };

  useEffect(() => {
    fetchFiles();
  }, [directory]);

  return (
    <div className="container">
      <div className="App">
        <div className="header">
          <h1>Welcome to Digital Declutter Assistant</h1>
          <h3>Your personal assistant to help you declutter your digital life.</h3>
        </div>
        
        <div style={{ margin: '20px 0' }}>
          <label>Directory: </label>
          <input 
            type="text" 
            value={directory}
            onChange={(e) => setDirectory(e.target.value)}
            style={{ width: '400px', padding: '5px' }}
          />
          <button onClick={fetchFiles} style={{ marginLeft: '10px' }}>Refresh</button>
        </div>

        <h2>Total Files: {files.length}</h2>
        <h2>Total Size: {formatSize(totalSize)}</h2>
        
        <div className="buttons">
          <button onClick={fetchFiles}>Refresh Files</button>
          <button onClick={() => {
            const filepath = prompt('Enter full file path to delete:');
            if (filepath) deleteFile(filepath);
          }}>Delete File</button>
        </div>

        <div style={{ margin: '20px 0' }}>
          <h3>Add New File</h3>
          <input 
            type="text"
            value={newFileName}
            onChange={(e) => setNewFileName(e.target.value)}
            placeholder="Enter filename (e.g., newfile.txt)"
            style={{ padding: '5px', width: '300px' }}
          />
          <button onClick={createFile} style={{ marginLeft: '10px' }}>Create File</button>
        </div>
      </div>
      
      <div className="files">
        <p><strong>Filesystem:</strong></p>
        <div style={{ maxHeight: '400px', overflow: 'auto' }}>
          {files.length === 0 ? (
            <p>No files found or backend not connected</p>
          ) : (
            <table style={{ width: '100%', borderCollapse: 'collapse' }}>
              <thead>
                <tr style={{ borderBottom: '2px solid #ccc' }}>
                  <th style={{ padding: '10px', textAlign: 'left' }}>Name</th>
                  <th style={{ padding: '10px', textAlign: 'left' }}>Type</th>
                  <th style={{ padding: '10px', textAlign: 'right' }}>Size</th>
                  <th style={{ padding: '10px', textAlign: 'center' }}>Actions</th>
                </tr>
              </thead>
              <tbody>
                {files.map((file, index) => (
                  <tr key={index} style={{ borderBottom: '1px solid #eee' }}>
                    <td style={{ padding: '10px' }}>{file.name}</td>
                    <td style={{ padding: '10px' }}>{file.type || 'N/A'}</td>
                    <td style={{ padding: '10px', textAlign: 'right' }}>{formatSize(file.size)}</td>
                    <td style={{ padding: '10px', textAlign: 'center' }}>
                      <button 
                        onClick={() => deleteFile(file.path)}
                        style={{ padding: '5px 10px', cursor: 'pointer' }}
                      >
                        Delete
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>
      </div>
    </div>
  );
}

export default App;