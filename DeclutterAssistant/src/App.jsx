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
      const normalizedDirectory = cleanupConfig.scanDirectory.replace(/\\/g, '/');
      const daysAgoTimestamp = Math.floor(Date.now() / 1000) - (cleanupConfig.daysOld * 24 * 60 * 60);
      
      console.log('Scan Request:', {
        directory: normalizedDirectory,
        fileType: cleanupConfig.fileType,
        timestamp: daysAgoTimestamp,
        date: new Date(daysAgoTimestamp * 1000).toLocaleString()
      });
      
      const response = await fetch(
        `${API_URL}/scan-cleanup?directory=${encodeURIComponent(normalizedDirectory)}&fileType=${encodeURIComponent(cleanupConfig.fileType)}&beforeTimestamp=${daysAgoTimestamp}`
      );
      
      const data = await response.json();
      console.log('Scan Response:', data);
      
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
      const normalizedDirectory = cleanupConfig.scanDirectory.replace(/\\/g, '/');
      const daysAgoTimestamp = Math.floor(Date.now() / 1000) - (cleanupConfig.daysOld * 24 * 60 * 60);
      
      const response = await fetch(`${API_URL}/cleanup`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          directory: normalizedDirectory,
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
            className="directory-input"
            value={directory}
            onChange={(e) => setDirectory(e.target.value)}
          />
          <button onClick={fetchFiles}>Refresh</button>
        </div>

        <h2>Total Files: {files.length}</h2>
        <h2>Total Size: {formatSize(totalSize)}</h2>
        
        <div className="button-group">
          <button onClick={fetchFiles}>Refresh Files</button>
          <button onClick={() => {
            const filepath = prompt('Enter full file path to delete:');
            if (filepath) deleteFile(filepath);
          }}>Delete File</button>
          <button onClick={() => setShowCleanupModal(true)}>Smart Cleanup</button>
        </div>

        <div className="add-file-section">
          <h3>Add New File</h3>
          <input 
            type="text"
            className="file-input"
            value={newFileName}
            onChange={(e) => setNewFileName(e.target.value)}
            placeholder="Enter filename (e.g., newfile.txt)"
          />
          <button onClick={createFile}>Create File</button>
        </div>
      </div>
      
      <div className="files">
        <p><strong>Filesystem:</strong></p>
        <div className="file-table-container">
          {files.length === 0 ? (
            <p>No files found or backend not connected</p>
          ) : (
            <table className="file-table">
              <thead>
                <tr className="table-header">
                  <th>Name</th>
                  <th>Type</th>
                  <th className="size-column">Size</th>
                  <th className="actions-column">Actions</th>
                </tr>
              </thead>
              <tbody>
                {files.map((file, index) => (
                  <tr key={index} className="table-row">
                    <td className="file-name">{file.name}</td>
                    <td className="file-type">{file.type || 'N/A'}</td>
                    <td className="size-column">{formatSize(file.size)}</td>
                    <td className="actions-column">
                      <button 
                        className="delete-btn"
                        onClick={() => deleteFile(file.path)}
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
        <div className="modal-overlay">
          <div className="modal-content">
            <h2 className="modal-header">Smart Cleanup</h2>
            
            <div className="form-group">
              <label className="form-label">Scan Directory:</label>
              <input 
                type="text"
                className="form-input"
                value={cleanupConfig.scanDirectory}
                onChange={(e) => setCleanupConfig({...cleanupConfig, scanDirectory: e.target.value})}
              />
            </div>

            <div className="form-group">
              <label className="form-label">File Type (e.g., .txt, .log, .tmp):</label>
              <input 
                type="text"
                className="form-input"
                value={cleanupConfig.fileType}
                onChange={(e) => setCleanupConfig({...cleanupConfig, fileType: e.target.value})}
                placeholder=".txt"
              />
            </div>

            <div className="form-group">
              <label className="form-label">Delete files older than (days):</label>
              <input 
                type="number"
                className="form-input"
                value={cleanupConfig.daysOld}
                onChange={(e) => setCleanupConfig({...cleanupConfig, daysOld: parseInt(e.target.value) || 0})}
                min="0"
              />
              <small className="form-hint">
                Files modified before {new Date(Date.now() - cleanupConfig.daysOld * 24 * 60 * 60 * 1000).toLocaleDateString()} will be deleted
              </small>
            </div>

            <div className="scan-button-group">
              <button 
                onClick={scanForCleanup}
                disabled={isScanning || isDeleting}
                className={`scan-button ${isScanning ? 'disabled' : ''}`}
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
                    <p className="sample-files-label">
                      <strong>Sample files (first 10):</strong>
                    </p>
                    <ul className="file-list">
                      {scanResults.files.slice(0, 10).map((file, idx) => (
                        <li key={idx} className="file-list-item">{file}</li>
                      ))}
                      {scanResults.files.length > 10 && (
                        <li className="file-list-more">
                          ... and {scanResults.files.length - 10} more files
                        </li>
                      )}
                    </ul>
                  </div>
                )}

                <button
                  onClick={executeCleanup}
                  disabled={isDeleting || scanResults.count === 0}
                  className={`delete-all-button ${(isDeleting || scanResults.count === 0) ? 'disabled' : ''}`}
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
              className={`close-button ${isDeleting ? 'disabled' : ''}`}
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