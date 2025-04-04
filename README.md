# 🔧 Custom Shell in C

A lightweight Unix-style shell built in C with support for command execution, background processing, redirection, and signal handling. This project was developed as part of a systems programming course to better understand process control, system calls, and how shells work under the hood.

---

## 🚀 Features

- ✅ **Command Execution**  
  Supports basic commands like `ls`, `pwd`, `cd`, and more.

- ✅ **Built-in Commands**  
  Includes built-ins such as:
  - `cd` – Change directories
  - `exit` – Exit the shell
  - `status` – View the exit status of the last foreground process

- ✅ **Background Execution**  
  Use `&` at the end of a command to run it in the background.

- ✅ **Input/Output Redirection**  
  Supports redirection using `<` and `>`.

- ✅ **Signal Handling**  
  Handles `SIGINT` (Ctrl+C) gracefully and prevents shell termination during foreground jobs.

---

## 🛠️ How It Works

- Uses `fork()`, `execvp()`, and `waitpid()` to manage child processes.
- Built-in commands are handled internally, without spawning new processes.
- Redirects standard input/output for file handling when `<` or `>` are used.
- Signal handlers are implemented using `sigaction()` for custom behavior.

---

## 📂 File Structure

