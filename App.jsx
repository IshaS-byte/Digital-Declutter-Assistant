import './App.css'

function App() {

  return (
    <div className="container">
      <div className="App">
        <div className="header">
          <h1>Welcome to Digital Declutter Assistant</h1>
          <h3>Your personal assistant to help you declutter your digital life.</h3>
        </div>
        <br/>
        <h2>Total Files: </h2>
        <h2>Total Size: </h2>
        <div className = "buttons">
          <button>File Viewer</button>
          <button>Delete File</button>
          <button>Add File</button>
          <button>Delete By Date</button>
        </div>
        
      </div>
      <div className="files">
          <p>Filesystem:</p>
      </div>
    </div>
  )
}

export default App
