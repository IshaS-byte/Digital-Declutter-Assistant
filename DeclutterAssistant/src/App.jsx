import { useState, useEffect } from 'react';
import './App.css';

function App() {
  const [files, setFiles] = useState([]);
  const [directory, setDirectory] = useState('C:\\');
  const [totalSize, setTotalSize] = useState(0);
  const [newFileName, setNewFileName] = useState('');
  const [showCleanupModal, setShowCleanupModal] = useState(false);
  const [cleanupConfig, setCleanupConfig] = useState({
    fileType: '',
    daysOld: 30,
    scanDirectory: 'C:\\'
  });
  const [scanResults, setScanResults] = useState(null);
  const [isScanning, setIsScanning] = useState(false);
  const [isDeleting, setIsDeleting] = useState(false);

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

  const scanForCleanup = async () => {
    if (!cleanupConfig.fileType.trim()) {
      alert('Please enter a file type (e.g., .txt, .log, .tmp)');
      return;
    }

    setIsScanning(true);
    setScanResults(null);

    try {
      const daysAgoTimestamp = Math.floor(Date.now() / 1000) - (cleanupConfig.daysOld * 24 * 60 * 60);
      
      const response = await fetch(
        `${API_URL}/scan-cleanup?directory=${encodeURIComponent(cleanupConfig.scanDirectory)}&fileType=${encodeURIComponent(cleanupConfig.fileType)}&beforeTimestamp=${daysAgoTimestamp}`
      );
      
      const data = await response.json();
      
      if (data.success) {
        setScanResults(data);
      } else {
        alert(data.message || 'Failed to scan directory');
      }
    } catch (error) {
      console.error('Error scanning for cleanup:', error);
      alert('Failed to scan directory. Make sure the C++ server is running.');
    } finally {
      setIsScanning(false);
    }
  };

  const executeCleanup = async () => {
    if (!scanResults || scanResults.count === 0) return;

    if (!window.confirm(`Are you sure you want to delete ${scanResults.count} file(s)? This action cannot be undone.`)) {
      return;
    }

    setIsDeleting(true);

    try {
      const daysAgoTimestamp = Math.floor(Date.now() / 1000) - (cleanupConfig.daysOld * 24 * 60 * 60);
      
      const response = await fetch(`${API_URL}/cleanup`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          directory: cleanupConfig.scanDirectory,
          fileType: cleanupConfig.fileType,
          beforeTimestamp: daysAgoTimestamp
        })
      });
      
      const data = await response.json();
      
      if (data.success) {
        alert(`Successfully deleted ${data.count} file(s)\nTotal space freed: ${formatSize(data.totalSize || 0)}`);
        setScanResults(null);
        setShowCleanupModal(false);
        fetchFiles();
      } else {
        alert(data.message || 'Failed to delete files');
      }
    } catch (error) {
      console.error('Error executing cleanup:', error);
      alert('Failed to execute cleanup operation');
    } finally {
      setIsDeleting(false);
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

        <div>
          <label className='directory-label'>Directory: </label>
          <input 
            type="text" 
            value={directory}
            onChange={(e) => setDirectory(e.target.value)}
            style={{ width: '400px', padding: '5px' }}
          />
          <button onClick={fetchFiles}>Refresh</button>
        </div>

        <h2>Total Files: {files.length}</h2>
        <h2>Total Size: {formatSize(totalSize)}</h2>
        
        <div>
          <button onClick={fetchFiles}>Refresh Files</button>
          <button onClick={() => {
            const filepath = prompt('Enter full file path to delete:');
            if (filepath) deleteFile(filepath);
          }}>Delete File</button>
          <button onClick={() => setShowCleanupModal(true)}> Smart Cleanup</button>
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

      {showCleanupModal && (
        <div style={{
          position: 'fixed',
          top: 0,
          left: 0,
          right: 0,
          bottom: 0,
          backgroundColor: 'rgba(0, 0, 0, 0.7)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 1000
        }}>
          <div style={{
            backgroundColor: '#CFE1B1',
            padding: '30px',
            borderRadius: '20px',
            maxWidth: '600px',
            width: '90%',
            maxHeight: '80vh',
            overflow: 'auto',
            boxShadow: '0 2rem 4rem rgba(0,0,0,0.3)',
            border: '2px solid black'
          }}>
            <h2 style={{ 
              backgroundColor: '#E1DBB1',
              padding: '15px',
              borderRadius: '10px',
              marginBottom: '20px',
              border: '2px solid black'
            }}>
              Smart Cleanup
            </h2>
            
            <div style={{ marginBottom: '20px' }}>
              <label style={{ display: 'block', marginBottom: '5px', fontWeight: 'bold' }}>
                Scan Directory:
              </label>
              <input 
                type="text"
                value={cleanupConfig.scanDirectory}
                onChange={(e) => setCleanupConfig({...cleanupConfig, scanDirectory: e.target.value})}
                style={{ 
                  width: '100%', 
                  padding: '10px',
                  fontSize: '16px',
                  borderRadius: '5px',
                  border: '1px solid #999'
                }}
              />
            </div>

            <div style={{ marginBottom: '20px' }}>
              <label style={{ display: 'block', marginBottom: '5px', fontWeight: 'bold' }}>
                File Type (e.g., .txt, .log, .tmp):
              </label>
              <input 
                type="text"
                value={cleanupConfig.fileType}
                onChange={(e) => setCleanupConfig({...cleanupConfig, fileType: e.target.value})}
                placeholder=".txt"
                style={{ 
                  width: '100%', 
                  padding: '10px',
                  fontSize: '16px',
                  borderRadius: '5px',
                  border: '1px solid #999'
                }}
              />
            </div>

            <div style={{ marginBottom: '20px' }}>
              <label style={{ display: 'block', marginBottom: '5px', fontWeight: 'bold' }}>
                Delete files older than (days):
              </label>
              <input 
                type="number"
                value={cleanupConfig.daysOld}
                onChange={(e) => setCleanupConfig({...cleanupConfig, daysOld: parseInt(e.target.value) || 0})}
                min="1"
                style={{ 
                  width: '100%', 
                  padding: '10px',
                  fontSize: '16px',
                  borderRadius: '5px',
                  border: '1px solid #999'
                }}
              />
            </div>

            <div style={{ marginBottom: '20px', display: 'flex', gap: '10px' }}>
              <button 
                onClick={scanForCleanup}
                disabled={isScanning || isDeleting}
                style={{
                  flex: 1,
                  padding: '12px',
                  fontSize: '16px',
                  backgroundColor: isScanning ? '#ccc' : '#B1E1CF',
                  cursor: isScanning ? 'not-allowed' : 'pointer',
                  opacity: isScanning ? 0.6 : 1
                }}
              >
                {isScanning ? 'Scanning...' : 'Scan for Files'}
              </button>
            </div>

            {scanResults && (
              <div className='results'>
                <h3>Scan Results:</h3>
                <p><strong>Files found:</strong> {scanResults.count}</p>
                <p><strong>Total size:</strong> {formatSize(scanResults.totalSize || 0)}</p>
                
                {scanResults.files && scanResults.files.length > 0 && (
                  <div>
                    <p style={{ marginTop: '10px', marginBottom: '5px' }}>
                      <strong>Sample files (first 10):</strong>
                    </p>
                    <ul style={{ 
                      maxHeight: '150px', 
                      overflow: 'auto',
                      fontSize: '12px',
                      padding: '10px',
                      backgroundColor: '#f5f5f5',
                      borderRadius: '5px'
                    }}>
                      {scanResults.files.slice(0, 10).map((file, idx) => (
                        <li key={idx} style={{ marginBottom: '5px' }}>{file}</li>
                      ))}
                      {scanResults.files.length > 10 && (
                        <li style={{ fontStyle: 'italic' }}>
                          ... and {scanResults.files.length - 10} more files
                        </li>
                      )}
                    </ul>
                  </div>
                )}

                <button
                  onClick={executeCleanup}
                  disabled={isDeleting || scanResults.count === 0}
                  style={{
                    width: '100%',
                    padding: '12px',
                    fontSize: '16px',
                    marginTop: '15px',
                    backgroundColor: isDeleting ? '#ccc' : '#E1B1B1',
                    cursor: (isDeleting || scanResults.count === 0) ? 'not-allowed' : 'pointer',
                    opacity: (isDeleting || scanResults.count === 0) ? 0.6 : 1,
                    fontWeight: 'bold'
                  }}
                >
                  {isDeleting ? 'Deleting...' : `Delete ${scanResults.count} File(s)`}
                </button>
              </div>
            )}

            <button
              onClick={() => {
                setShowCleanupModal(false);
                setScanResults(null);
              }}
              disabled={isDeleting}
              style={{
                width: '94%',
                padding: '12px',
                fontSize: '16px',
                backgroundColor: '#E1C3B1',
                cursor: isDeleting ? 'not-allowed' : 'pointer',
                opacity: isDeleting ? 0.6 : 1
              }}
            >
              Close
            </button>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;