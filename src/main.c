#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Variáveis de Controle do Jogo Atual
static char currentGameName[64] = "Nenhum jogo selecionado";
static char currentGameUrl[128] = "";
static char currentGameFile[64] = "jogo.iso";

// Função para Baixar Arquivos (ISO ou TXT) da Rede
int download_file(const char *server_ip, int port, const char *url_path, const char *dest_path) 
{
    int sock;
    struct sockaddr_in server_addr;
    char request[512];
    char buffer[4096];
    FILE *f;
    int bytes_received;

    f = fopen(dest_path, "wb");
    if (!f) return -1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        fclose(f);
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fclose(f);
        return -1;
    }

    snprintf(request, sizeof(request), 
             "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", 
             url_path, server_ip);
    send(sock, request, strlen(request), 0);

    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, f);
    }

    fclose(f);
    disconnect(sock);
    return 0;
}

// Executado ao Apertar (X) CONECTAR / BAIXAR
static void handle_input(void)
{
    u32 p = read_pad_pressed();

    if (p & PAD_CROSS) {
        snprintf(statusLine, sizeof(statusLine), "Baixando %s para mass:/DVD/...", currentGameFile);
        render(); // Atualiza a tela informando o inicio

        char destPath[256];
        snprintf(destPath, sizeof(destPath), "mass:/DVD/%s", currentGameFile);

        // Dispara o download direto para a pasta DVD do pendrive
        if (download_file("192.168.1.100", 80, currentGameUrl, destPath) == 0) {
            snprintf(statusLine, sizeof(statusLine), "Download Concluido! Abra o OPL.");
        } else {
            snprintf(statusLine, sizeof(statusLine), "Erro no download. Verifique a rede.");
        }
    }
}
