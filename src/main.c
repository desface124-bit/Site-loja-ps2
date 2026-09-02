#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <libpad.h>
#include <gsKit.h>
#include <gsFontM.h>
#include <dmaKit.h>
#include <gsInline.h>

// Definição de Cores Auxiliar
#define rgba(r, g, b, a) GS_SETREG_RGBA(r, g, b, a)

// Ponteiros Globais do gsKit
static GSGLOBAL *gsGlobal = NULL;
static GSFONTM *fontm = NULL;

// Variáveis Globais de Controle e Estado
static u32 oldPad = 0;
static char statusLine[128] = "Pronto.";
static char targetUrl[128] = "https://sualoja.com";
static char storeName[64] = "LOJA PS2 WEB";
static char videoModeStr[32] = "480p (60Hz)";
static int videoModeVal = GS_MODE_DTV_480P;

// Protótipos de Funções Auxiliares
void init_pad(void) {}
u32 read_pad_raw(void) { return 0; }

// Função para remover espaços em branco do início e fim da string
static char* trim(char *str)
{
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Mapeia a string lida do CFG para os modos de vídeo do gsKit
static void set_video_mode_from_str(const char *modeStr)
{
    if (strcasecmp(modeStr, "720p") == 0) {
        videoModeVal = GS_MODE_DTV_720P;
        snprintf(videoModeStr, sizeof(videoModeStr), "720p (HD)");
    } else if (strcasecmp(modeStr, "1080i") == 0) {
        videoModeVal = GS_MODE_DTV_1080I;
        snprintf(videoModeStr, sizeof(videoModeStr), "1080i (HD)");
    } else if (strcasecmp(modeStr, "480p") == 0) {
        videoModeVal = GS_MODE_DTV_480P;
        snprintf(videoModeStr, sizeof(videoModeStr), "480p (Progressivo)");
    } else if (strcasecmp(modeStr, "pal") == 0) {
        videoModeVal = GS_MODE_PAL;
        snprintf(videoModeStr, sizeof(videoModeStr), "PAL (50Hz)");
    } else if (strcasecmp(modeStr, "ntsc") == 0) {
        videoModeVal = GS_MODE_NTSC;
        snprintf(videoModeStr, sizeof(videoModeStr), "NTSC (60Hz)");
    }
}

// Função para Carregar e Interpretar a Configuração com Rótulos do Pendrive
void load_config(void)
{
    FILE *f = fopen("mass:/SITEPS2/CONFIG.CFG", "rb");
    if (f == NULL) {
        f = fopen("mass:/CONFIG.CFG", "rb");
    }

    if (f != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), f) != NULL) {
            // Remove quebras de linha
            line[strcspn(line, "\r\n")] = 0;
            
            char *cleanLine = trim(line);
            if (strlen(cleanLine) == 0 || cleanLine[0] == '#') continue; // Ignora linhas vazias/comentários

            // Procura o caractere ':'
            char *sep = strchr(cleanLine, ':');
            if (sep != NULL) {
                *sep = '\0'; // Separa chave e valor
                char *key = trim(cleanLine);
                char *val = trim(sep + 1);

                // Reconstitui o valor para URLs (ex: https://...)
                if (sep[1] == '/' && sep[2] == '/') {
                    *(sep) = ':';
                }

                if (strcasecmp(key, "Site") == 0) {
                    snprintf(targetUrl, sizeof(targetUrl), "%s", val);
                } else if (strcasecmp(key, "Vídeo mode") == 0 || strcasecmp(key, "Video mode") == 0) {
                    set_video_mode_from_str(val);
                }
            }
        }
        snprintf(statusLine, sizeof(statusLine), "Configuracao e Video lidos do USB!");
        fclose(f);
    } else {
        snprintf(statusLine, sizeof(statusLine), "CONFIG.CFG nao encontrado. Usando padrao.");
    }
}

// Função para Desenhar Retângulos/Caixas Usando gsKit NATIVO
static void drawBox(float x1, float y1, float x2, float y2, u64 color)
{
    gsKit_prim_sprite(gsGlobal, x1, y1, x2, y2, 1, color);
}

// Função para Desenhar Texto Usando gsFontM NATIVO
static void drawText(float x, float y, float scale, u64 color, const char *text)
{
    if (fontm != NULL) {
        gsKit_fontm_print(gsGlobal, fontm, x, y, 1, color, text);
    }
}

static u32 read_pad_pressed(void)
{
    u32 padData = read_pad_raw();
    u32 pressed = padData & ~oldPad;
    oldPad = padData;

    return pressed;
}

static void handle_input(void)
{
    u32 p = read_pad_pressed();

    if (p & PAD_CROSS) {
        snprintf(statusLine, sizeof(statusLine), "Carregando: %s", targetUrl);
    }
}

static void render(void)
{
    float w = (float)gsGlobal->Width;
    float h = (float)gsGlobal->Height;

    // Palette Light Mode (Fundo Claro / Texto Escuro)
    u64 bg = rgba(245, 247, 250, 255);       // Fundo branco/gelo
    u64 topBar = rgba(255, 255, 255, 255);   // Barra superior branca
    u64 border = rgba(210, 215, 225, 255);   // Linhas e bordas cinza claro
    u64 cardBg = rgba(255, 255, 255, 255);   // Painel principal
    u64 textDark = rgba(20, 24, 33, 255);    // Texto preto/grafite
    u64 textMuted = rgba(100, 110, 125, 255);// Texto cinza
    u64 accentBlue = rgba(24, 119, 242, 255);// Azul Destaque
    u64 accentGreen = rgba(34, 197, 94, 255);// Verde Status

    gsKit_clear(gsGlobal, bg);

    // Top Bar (Branca com linha divisória cinza)
    drawBox(0, 0, w, 50, topBar);
    drawBox(0, 49, w, 50, border);
    drawText(20, 12, 0.75f, textDark, storeName);
    drawText(w - 140, 16, 0.50f, accentBlue, "[ WEB MODE ]");

    // Container Central
    drawBox(20, 70, w - 20, h - 50, cardBg);
    drawBox(20, 70, w - 20, 71, border);
    drawBox(20, h - 50, w - 20, h - 49, border);

    // Símbolo/Ícone de Web (Desenhado via Sprite)
    drawBox(40, 90, 56, 106, accentBlue);
    drawText(65, 90, 0.65f, textDark, "ENDERECO CONFIGURADO:");

    // Input Box do Link (Estilo caixa de navegação)
    drawBox(40, 120, w - 40, 160, bg);
    drawBox(40, 120, w - 40, 121, border);
    drawBox(40, 159, w - 40, 160, border);
    drawText(50, 132, 0.55f, textDark, targetUrl);

    // Botões de Ação Simulado
    drawBox(40, 180, 180, 215, accentBlue);
    drawText(60, 190, 0.50f, topBar, "(X) CONECTAR");

    drawBox(190, 180, 310, 215, border);
    drawText(205, 190, 0.50f, textDark, "(O) SAIR");

    // Informações do Sistema
    drawText(40, 235, 0.48f, textMuted, "Arquivo Lido:");
    drawText(150, 235, 0.48f, textDark, "mass:/SITEPS2/CONFIG.CFG");

    drawText(40, 255, 0.48f, textMuted, "Modo de Video:");
    drawText(150, 255, 0.48f, accentGreen, videoModeStr);

    // Bottom Status Bar
    drawBox(0, h - 35, w, h, topBar);
    drawBox(0, h - 35, w, h - 34, border);
    drawText(15, h - 25, 0.48f, textMuted, statusLine);

    gsKit_queue_exec(gsGlobal);
    gsKit_sync_flip(gsGlobal);
}

static int init_graphics(void)
{
    gsGlobal = gsKit_init_global();
    if (!gsGlobal) return -1;

    gsGlobal->Mode = videoModeVal;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_PERSISTENT);
    gsKit_clear(gsGlobal, rgba(255, 255, 255, 0));

    fontm = gsKit_init_fontm();
    if (fontm != NULL) {
        gsKit_fontm_upload(gsGlobal, fontm);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int ret;
    (void)argc; (void)argv;

    SifInitRpc(0);

    // Carrega e interpreta a configuração do USB
    load_config();

    // Inicializa a tela gráfica com o modo de vídeo lido do CFG
    ret = init_graphics();
    if (ret < 0) {
        SleepThread();
        return 1;
    }

    init_pad();

    for (;;) {
        handle_input();
        render();
    }

    return 0;
}
