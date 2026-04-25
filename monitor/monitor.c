#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

// MUDE AQUI PARA A PORTA DO SEU ESP32
#define PORTA_SERIAL "/dev/ttyACM0" 
#define BAUDRATE B115200

// --- FUNÇÕES BASH ---

// Pega o uso da CPU (Soma o uso de Usuário + Sistema do comando top)
int get_cpu_usage() {
    FILE *fp;
    char buffer[32];
    int cpu_usage = 0;
    
    // Roda o top 2 vezes rápido (-bn2) e pega a segunda linha pq a primeira é a média desde o boot
    fp = popen("top -bn2 -d 0.1 | grep 'Cpu(s)' | tail -n 1 | awk '{printf \"%.0f\", $2 + $4}'", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            cpu_usage = atoi(buffer);
        }
        pclose(fp);
    }
    return cpu_usage;
}

// Pega a % de RAM usada usando o comando free
int get_ram_usage() {
    FILE *fp;
    char buffer[32];
    int ram_usage = 0;
    
    fp = popen("free | grep Mem | awk '{printf \"%.0f\", ($3/$2)*100}'", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            ram_usage = atoi(buffer);
        }
        pclose(fp);
    }
    return ram_usage;
}

// Pega os 5 processos que mais consomem RAM
// Formato de saída do bash: nome1,ram1,nome2,ram2...
void get_top_processes(char* output_buffer, int max_len) {
    FILE *fp;
    char line[128];
    output_buffer[0] = '\0'; // Limpa o buffer
    
    // ps lista processos, ordena por RAM (-rss). Pegamos os 5 primeiros (ignorando o cabeçalho)
    fp = popen("ps -eo comm,rss --sort=-rss | head -n 6 | tail -n 5 | awk '{printf \"%s,%d,\", $1, $2/1024}'", "r");
    if (fp != NULL) {
        if (fgets(line, sizeof(line), fp) != NULL) {
            // Remove a última vírgula e concatena
            line[strlen(line)-1] = '\0'; 
            strncat(output_buffer, line, max_len - 1);
        }
        pclose(fp);
    }
}

// --- CONFIGURAÇÃO DA PORTA SERIAL ---
int init_serial(const char* portname) {
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Erro ao abrir a porta serial %s. Tem permissão de root/dialout?\n", portname);
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        printf("Erro tcgetattr\n");
        return -1;
    }

    cfsetospeed(&tty, BAUDRATE);
    cfsetispeed(&tty, BAUDRATE);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_cflag |= (CLOCAL | CREAD);            // Ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);          // Sem paridade
    tty.c_cflag &= ~CSTOPB;                     // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                    // Sem controle de fluxo em hardware

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // Desliga controle de fluxo software
    tty.c_lflag = 0;                            // Modo não canônico (raw)
    tty.c_oflag = 0;                            // Saída raw

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Erro tcsetattr\n");
        return -1;
    }
    
    // Limpa o buffer da serial antes de começar
    tcflush(fd, TCIOFLUSH);
    return fd;
}

// --- MAIN ---
int main() {
    printf("Iniciando Monitor de Hardware...\n");
    
    int serial_fd = init_serial(PORTA_SERIAL);
    if (serial_fd < 0) return 1;

    char serial_buffer[256];
    char proc_buffer[128];

    while (1) {
        int cpu = get_cpu_usage();
        int ram = get_ram_usage();
        get_top_processes(proc_buffer, sizeof(proc_buffer));

        // Monta a string final com o protocolo: C:xx;R:xx;P:proc1,ram1,proc2,ram2;
        snprintf(serial_buffer, sizeof(serial_buffer), "C:%d;R:%d;P:%s;\n", cpu, ram, proc_buffer);
        
        // Imprime no console do PC pra vc ver o que está enviando
        printf("Enviando -> %s", serial_buffer);

        // Escreve na porta serial pro ESP32
        write(serial_fd, serial_buffer, strlen(serial_buffer));

        // Espera 2 segundos antes de ler de novo
        sleep(2);
    }

    close(serial_fd);
    return 0;
}