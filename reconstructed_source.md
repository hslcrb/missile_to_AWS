# Reconstructed Source Code Analysis

Based on the binary analysis (decompilation) of [bomb.exe](file:///f:/aws-nuke/bomb.exe), here is a reconstruction of the core logic. Note that this is a symbolic representation as the original variable names and comments are lost during minification/compilation.

## Backend (Rust / Tauri)

The backend is responsible for file I/O and executing the `aws-nuke` logic.

### `src-tauri/src/main.rs` (Conceptual)
```rust
#[tauri::command]
async fn run_nuke(account_id: String, regions: Vec<String>) -> Result<String, String> {
    // 1. Load credentials from external/credentials.json
    // 2. Build the command for aws-nuke
    let mut command = std::process::Command::new("external/aws-nuke");
    command.arg("--config").arg("external/config.yaml");
    command.arg("--access-key-id").arg(aws_key);
    command.arg("--secret-access-key").arg(aws_secret);
    command.arg("--no-dry-run"); 
    command.arg("--force");
    
    // 3. Execute and capture output
    let output = command.output().map_err(|e| e.to_string())?;
    Ok(String::from_utf8_lossy(&output.stdout).to_string())
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![run_nuke])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
```

## Frontend (React / Next.js)

The frontend provides the UI for entering the Account ID and triggering the operation.

### `src/pages/index.tsx` (Conceptual)
```javascript
import { useState } from 'react';
import { invoke } from '@tauri-apps/api/tauri';

export default function Home() {
  const [accountId, setAccountId] = useState('');
  const [status, setStatus] = useState('');

  const handleNuke = async () => {
    try {
      setStatus('Nuking started...');
      const result = await invoke('run_nuke', { accountId });
      setStatus(`Success: ${result}`);
    } catch (err) {
      setStatus(`Error: ${err}`);
    }
  };

  return (
    <div className="container">
      <h1>AWS Nuke Tool</h1>
      <input 
        type="text" 
        placeholder="자신의 AWS ID를 적으세요" 
        value={accountId}
        onChange={(e) => setAccountId(e.target.value)}
      />
      <button onClick={handleNuke}>BOOM</button>
      <div className="status">{status}</div>
    </div>
  );
}
```

## Internal Assets

- **Config**: Referenced as [external/config.yaml](file:///f:/aws-nuke/external/config.yaml).
- **Credentials**: Referenced as [external/credentials.json](file:///f:/aws-nuke/external/credentials.json).
- **Framework**: Built using Next.js (SSG mode) and bundled into the Tauri binary using the `tauri` CLI.

## Summary of Logic
The application acts as a graphical wrapper around `aws-nuke`. It eliminates the need for the command line by providing a simple input field for the Account ID and a button to trigger the process. It reads sensitive credentials from a local JSON file to authenticate against AWS APIs.
