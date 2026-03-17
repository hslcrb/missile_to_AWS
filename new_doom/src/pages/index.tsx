import { useState } from 'react';
import { invoke } from '@tauri-apps/api/tauri';
import Head from 'next/head';

export default function Home() {
  const [accountId, setAccountId] = useState('');
  const [status, setStatus] = useState('');
  const [loading, setLoading] = useState(false);

  const handleNuke = async () => {
    if (!accountId) {
      alert('AWS ID(Account ID)를 입력해주세요.');
      return;
    }

    const confirmNuke = confirm('정말로 모든 리소스를 삭제하시겠습니까? 이 작업은 되돌릴 수 없습니다.');
    if (!confirmNuke) return;

    setLoading(true);
    setStatus('작업 중...');

    try {
      const response = await invoke('run_nuke', { accountId });
      setStatus(response as string);
    } catch (error) {
      setStatus(`에러 발생: ${error}`);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="container">
      <Head>
        <title>AWS Nuke Tool</title>
      </Head>

      <main>
        <h1>AWS Nuke Tool</h1>
        <p className="description">
          지정된 계정의 모든 리소스를 삭제합니다.
        </p>

        <div className="card">
          <label htmlFor="accountId">Account ID</label>
          <input
            id="accountId"
            type="text"
            placeholder="자신의 AWS ID를 입력하세요"
            value={accountId}
            onChange={(e) => setAccountId(e.target.value)}
            disabled={loading}
          />

          <button 
            className="nuke-button" 
            onClick={handleNuke} 
            disabled={loading}
          >
            {loading ? '작업 중...' : 'BOOM'}
          </button>
        </div>

        {status && (
          <div className={`status-box ${status.includes('에러') ? 'error' : 'success'}`}>
            {status}
          </div>
        )}
      </main>

      <style jsx>{`
        .container {
          min-height: 100vh;
          padding: 0 0.5rem;
          display: flex;
          flex-direction: column;
          justify-content: center;
          align-items: center;
          background: #1a1a1a;
          color: white;
          font-family: -apple-system, BlinkMacSystemFont, Segoe UI, Roboto, Oxygen,
            Ubuntu, Cantarell, Fira Sans, Droid Sans, Helvetica Neue, sans-serif;
        }

        main {
          padding: 5rem 0;
          flex: 1;
          display: flex;
          flex-direction: column;
          justify-content: center;
          align-items: center;
        }

        h1 {
          margin: 0;
          line-height: 1.15;
          font-size: 4rem;
          text-align: center;
          background: linear-gradient(to right, #ff416c, #ff4b2b);
          -webkit-background-clip: text;
          -webkit-text-fill-color: transparent;
        }

        .description {
          text-align: center;
          line-height: 1.5;
          font-size: 1.5rem;
          margin-top: 1rem;
          color: #888;
        }

        .card {
          margin: 2rem;
          padding: 2rem;
          text-align: left;
          color: inherit;
          text-decoration: none;
          border: 1px solid #333;
          border-radius: 10px;
          background: #252525;
          width: 400px;
          display: flex;
          flex-direction: column;
        }

        label {
          margin-bottom: 0.5rem;
          font-weight: bold;
          color: #aaa;
        }

        input {
          padding: 0.8rem;
          margin-bottom: 1.5rem;
          border-radius: 5px;
          border: 1px solid #444;
          background: #333;
          color: white;
          font-size: 1rem;
        }

        .nuke-button {
          padding: 1rem;
          border-radius: 5px;
          border: none;
          background: #ff4b2b;
          color: white;
          font-size: 1.2rem;
          font-weight: bold;
          cursor: pointer;
          transition: background 0.3s;
        }

        .nuke-button:hover:not(:disabled) {
          background: #ff3111;
        }

        .nuke-button:disabled {
          opacity: 0.6;
          cursor: not-allowed;
        }

        .status-box {
          margin-top: 2rem;
          padding: 1rem;
          border-radius: 5px;
          width: 400px;
          text-align: center;
        }

        .success {
          background: #1b4d3e;
          border: 1px solid #2ecc71;
        }

        .error {
          background: #4d1b1b;
          border: 1px solid #e74c3c;
        }
      `}</style>

      <style jsx global>{`
        html,
        body {
          padding: 0;
          margin: 0;
          background: #1a1a1a;
        }

        * {
          box-sizing: border-box;
        }
      `}</style>
    </div>
  );
}
