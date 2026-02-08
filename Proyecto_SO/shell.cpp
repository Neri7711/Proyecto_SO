#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>
#include <glob.h>

using namespace std;

class MiniShell {
private:
    string current_dir;
    vector<pid_t> background_processes;
    
    void signal_handler(int sig) {
        if (sig == SIGINT) {
            cout << "\nMiniShell> ";
            cout.flush();
        } else if (sig == SIGTSTP) {
            cout << "\nMiniShell> ";
            cout.flush();
        }
    }
    
    vector<string> tokenize(const string& line) {
        vector<string> tokens;
        stringstream ss(line);
        string token;
        
        while (ss >> token) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    bool is_builtin(const string& cmd) {
        return cmd == "cd" || cmd == "exit" || cmd == "help" || cmd == "pwd";
    }
    
    int execute_builtin(const vector<string>& args) {
        if (args[0] == "cd") {
            if (args.size() < 2) {
                const char* home = getenv("HOME");
                if (home) {
                    chdir(home);
                }
            } else {
                if (chdir(args[1].c_str()) != 0) {
                    perror("cd");
                }
            }
            return 0;
        } else if (args[0] == "exit") {
            exit(0);
        } else if (args[0] == "help") {
            cout << "MiniShell - Comandos integrados:\n";
            cout << "  cd <dir>  - Cambiar directorio\n";
            cout << "  pwd       - Mostrar directorio actual\n";
            cout << "  exit      - Salir del shell\n";
            cout << "  help      - Mostrar esta ayuda\n";
            cout << "Soporta redirección (<, >, >>), pipes (|) y procesos en segundo plano (&)\n";
            return 0;
        } else if (args[0] == "pwd") {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                cout << cwd << endl;
            } else {
                perror("pwd");
            }
            return 0;
        }
        return -1;
    }
    
    vector<vector<string>> parse_pipeline(const vector<string>& tokens) {
        vector<vector<string>> pipeline;
        vector<string> current_cmd;
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == "|") {
                if (!current_cmd.empty()) {
                    pipeline.push_back(current_cmd);
                    current_cmd.clear();
                }
            } else {
                current_cmd.push_back(tokens[i]);
            }
        }
        
        if (!current_cmd.empty()) {
            pipeline.push_back(current_cmd);
        }
        
        return pipeline;
    }
    
    void handle_redirection(vector<string>& args, string& input_file, string& output_file, bool& append) {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "<") {
                if (i + 1 < args.size()) {
                    input_file = args[i + 1];
                    args.erase(args.begin() + i, args.begin() + i + 2);
                    i--;
                }
            } else if (args[i] == ">") {
                if (i + 1 < args.size()) {
                    output_file = args[i + 1];
                    append = false;
                    args.erase(args.begin() + i, args.begin() + i + 2);
                    i--;
                }
            } else if (args[i] == ">>") {
                if (i + 1 < args.size()) {
                    output_file = args[i + 1];
                    append = true;
                    args.erase(args.begin() + i, args.begin() + i + 2);
                    i--;
                }
            }
        }
    }
    
    void execute_command(vector<string> args, string input_file = "", string output_file = "", bool append = false, bool background = false) {
        if (args.empty()) return;
        
        if (is_builtin(args[0])) {
            execute_builtin(args);
            return;
        }
        
        pid_t pid = fork();
        
        if (pid == 0) {
            if (!input_file.empty()) {
                int fd_in = open(input_file.c_str(), O_RDONLY);
                if (fd_in < 0) {
                    perror("open input file");
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            
            if (!output_file.empty()) {
                int flags = O_WRONLY | O_CREAT;
                flags |= append ? O_APPEND : O_TRUNC;
                int fd_out = open(output_file.c_str(), flags, 0644);
                if (fd_out < 0) {
                    perror("open output file");
                    exit(1);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            
            vector<char*> argv;
            for (auto& arg : args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            
            execvp(argv[0], argv.data());
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            if (background) {
                background_processes.push_back(pid);
                cout << "[" << background_processes.size() << "] " << pid << endl;
            } else {
                int status;
                waitpid(pid, &status, 0);
            }
        } else {
            perror("fork");
        }
    }
    
    void execute_pipeline(vector<vector<string>> pipeline, string input_file = "", string output_file = "", bool append = false, bool background = false) {
        if (pipeline.size() == 1) {
            execute_command(pipeline[0], input_file, output_file, append, background);
            return;
        }
        
        int pipefd[2];
        pid_t pid;
        int prev_pipe_read = -1;
        
        if (!input_file.empty()) {
            prev_pipe_read = open(input_file.c_str(), O_RDONLY);
            if (prev_pipe_read < 0) {
                perror("open input file");
                return;
            }
        }
        
        for (size_t i = 0; i < pipeline.size(); ++i) {
            if (i < pipeline.size() - 1) {
                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    return;
                }
            }
            
            pid = fork();
            
            if (pid == 0) {
                if (prev_pipe_read != -1) {
                    dup2(prev_pipe_read, STDIN_FILENO);
                    close(prev_pipe_read);
                }
                
                if (i < pipeline.size() - 1) {
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[0]);
                    close(pipefd[1]);
                }
                
                if (i == pipeline.size() - 1 && !output_file.empty()) {
                    int flags = O_WRONLY | O_CREAT;
                    flags |= append ? O_APPEND : O_TRUNC;
                    int fd_out = open(output_file.c_str(), flags, 0644);
                    if (fd_out < 0) {
                        perror("open output file");
                        exit(1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                }
                
                vector<char*> argv;
                for (auto& arg : pipeline[i]) {
                    argv.push_back(const_cast<char*>(arg.c_str()));
                }
                argv.push_back(nullptr);
                
                execvp(argv[0], argv.data());
                perror("execvp");
                exit(1);
            } else if (pid > 0) {
                if (prev_pipe_read != -1) {
                    close(prev_pipe_read);
                }
                
                if (i < pipeline.size() - 1) {
                    close(pipefd[1]);
                    prev_pipe_read = pipefd[0];
                }
                
                if (!background) {
                    if (i == pipeline.size() - 1) {
                        int status;
                        waitpid(pid, &status, 0);
                    }
                }
            } else {
                perror("fork");
                return;
            }
        }
        
        if (background) {
            background_processes.push_back(pid);
            cout << "[" << background_processes.size() << "] " << pid << endl;
        }
    }
    
    void cleanup_background() {
        for (auto it = background_processes.begin(); it != background_processes.end(); ) {
            int status;
            pid_t result = waitpid(*it, &status, WNOHANG);
            
            if (result == *it) {
                cout << "[" << distance(background_processes.begin(), it) + 1 << "] Done " << *it << endl;
                it = background_processes.erase(it);
            } else if (result == -1) {
                it = background_processes.erase(it);
            } else {
                ++it;
            }
        }
    }
    
public:
    MiniShell() {
        signal(SIGINT, signal_handler);
        signal(SIGTSTP, signal_handler);
        
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            current_dir = cwd;
        }
    }
    
    void run() {
        string line;
        
        while (true) {
            cleanup_background();
            
            char cwd[1024];
            string prompt = "MiniShell";
            if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                prompt += ":";
                prompt += cwd;
                prompt += "$ ";
            } else {
                prompt += "$ ";
            }
            
            cout << prompt;
            cout.flush();
            
            if (!getline(cin, line)) {
                cout << endl;
                break;
            }
            
            if (line.empty()) continue;
            
            vector<string> tokens = tokenize(line);
            if (tokens.empty()) continue;
            
            bool background = false;
            if (tokens.back() == "&") {
                background = true;
                tokens.pop_back();
            }
            
            string input_file, output_file;
            bool append = false;
            
            vector<vector<string>> pipeline = parse_pipeline(tokens);
            
            if (!pipeline.empty()) {
                handle_redirection(pipeline.front(), input_file, output_file, append);
                if (pipeline.size() == 1 && !output_file.empty()) {
                    handle_redirection(pipeline.front(), input_file, output_file, append);
                }
            }
            
            execute_pipeline(pipeline, input_file, output_file, append, background);
        }
    }
};

int main() {
    MiniShell shell;
    shell.run();
    return 0;
}
