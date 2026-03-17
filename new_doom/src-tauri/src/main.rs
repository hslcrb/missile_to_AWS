use serde::{Deserialize, Serialize};
use std::process::Command;
use std::fs;
use tauri::Manager;

#[derive(Serialize, Deserialize)]
struct Credentials {
    aws_access_key_id: String,
    aws_secret_access_key: String,
}

#[tauri::command]
async fn run_nuke(account_id: String) -> Result<String, String> {
    // 1. Read credentials
    let creds_raw = fs::read_to_string("external/credentials.json")
        .map_err(|e| format!("Failed to read credentials: {}", e))?;
    let creds: Credentials = serde_json::from_str(&creds_raw)
        .map_err(|e| format!("Failed to parse credentials: {}", e))?;

    // 2. Prepare command
    // Based on decompiled strings, it uses external/aws-nuke
    let mut child = Command::new("external/aws-nuke.exe")
        .arg("--config")
        .arg("external/config.yaml")
        .arg("--access-key-id")
        .arg(&creds.aws_access_key_id)
        .arg("--secret-access-key")
        .arg(&creds.aws_secret_access_key)
        .arg("--no-dry-run")
        .arg("--force")
        .spawn()
        .map_err(|e| format!("Failed to start aws-nuke: {}", e))?;

    let status = child.wait().map_err(|e| format!("Error waiting for process: {}", e))?;

    if status.success() {
        Ok("Successfully nuked resources.".into())
    } else {
        Err(format!("aws-nuke exited with status: {}", status))
    }
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![run_nuke])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
