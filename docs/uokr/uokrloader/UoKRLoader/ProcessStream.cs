using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Runtime.InteropServices;

namespace UoKRLoader
{
    class ProcessStream : Stream
    {
        private bool m_Open;
        private int m_Position;
        private IntPtr m_Process;
        private IntPtr m_ProcessID;
        private const int ProcessAllAccess = 0x1f0fff;

        public bool IsOpened
        {
            get
            {
                return this.m_Open;
            }
            set
            {
                this.m_Open = value;
            }
        }

        public IntPtr ProcessHandle
        {
            get
            {
                return this.m_Process;
            }
            set
            {
                this.m_Process = value;
            }
        }

        public ProcessStream()
        {

        }

        public ProcessStream(IntPtr processID)
        {
            this.m_ProcessID = processID;
            BeginAccess();
        }

        public static ProcessStream GetProcessFromHandle(IntPtr handle)
        {
            ProcessStream pcsStream = new ProcessStream();
            pcsStream.ProcessHandle = handle;
            pcsStream.IsOpened = true;

            return pcsStream;
        }

        public override void Close()
        {
            EndAccess();
        }

        public bool BeginAccess()
        {
            if (this.m_Open)
            {
                return false;
            }

            this.m_Process = OpenProcess(ProcessAllAccess, 0, this.m_ProcessID);
            this.m_Open = true;
            return true;
        }

        public void EndAccess()
        {
            if (this.m_Open)
            {
                CloseHandle(this.m_Process);
                this.m_Open = false;
            }
        }

        public override void Flush()
        {

        }

        public override unsafe int Read(byte[] buffer, int offset, int count)
        {
            bool flag = !this.BeginAccess();
            int op = 0;
            fixed (byte* numRef = buffer)
            {
                ReadProcessMemory(this.m_Process, this.m_Position, (void*)(numRef + offset), count, ref op);
            }
            this.m_Position += count;
            if (flag)
            {
                this.EndAccess();
            }
            return op;
        }

        public override unsafe void Write(byte[] buffer, int offset, int count)
        {
            bool flag = !this.BeginAccess();
            fixed (byte* numRef = buffer)
            {
                WriteProcessMemory(this.m_Process, this.m_Position, (void*)(numRef + offset), count, 0);
            }
            this.m_Position += count;
            if (flag)
            {
                this.EndAccess();
            }
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            switch (origin)
            {
                case SeekOrigin.Begin:
                    this.m_Position = (int)offset;
                    break;

                case SeekOrigin.Current:
                    this.m_Position += (int)offset;
                    break;

                case SeekOrigin.End:
                    throw new NotSupportedException();
            }
            return (long)this.m_Position;
        }

        public override void SetLength(long value)
        {
            throw new NotSupportedException();
        }

        public override bool CanRead
        {
            get { return this.m_Open; }
        }

        public override bool CanSeek
        {
            get { return this.m_Open; }
        }

        public override bool CanWrite
        {
            get { return this.m_Open; }
        }

        public override long Length
        {
            get { throw new NotSupportedException(); }
        }

        public override long Position
        {
            get
            {
                return (long)this.m_Position;
            }
            set
            {
                Seek(value, SeekOrigin.Begin);
            }
        }

        [DllImport("Kernel32")]
        private static extern int CloseHandle(IntPtr handle);

        [DllImport("Kernel32")]
        private static extern IntPtr OpenProcess(int desiredAccess, int inheritHandle, IntPtr processID);

        [DllImport("Kernel32")]
        private static extern unsafe int ReadProcessMemory(IntPtr process, int baseAddress, void* buffer, int size, ref int op);

        [DllImport("Kernel32")]
        private static extern unsafe int WriteProcessMemory(IntPtr process, int baseAddress, void* buffer, int size, int nullMe);
    }
}
