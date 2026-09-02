#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <tamtypes.h>
#include <kernel.h>
#include <delaythread.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>

#include <gsKit.h>
#include <dmaKit.h>
#include <gsFontM.h>

#include "config.h"

#define PAD_PORT 0
#define PAD_SLOT 0
#define PAD_AREA_SIZE 256
#define MAX_URL 191
#define MAX_NAME 63

static unsigned char padArea[PAD_AREA_SIZE] __attribute__((aligned(64)));
static GSGLOBAL *gsGlobal = NULL;
static GSFONTM *fontm = NULL;

static char targetUrl[MAX_URL + 1] = "http://localhost";
static char storeName[MAX_NAME + 1] = "NAVEGADOR PS2";
static char videoModeStr[32] = "NTSC";
static int videoModeVal = GS_MODE_NTSC;

static char statusLine[256] = "Pressione (X) para conectar ao site configurado.";

static void copy_string(char *dst, size_t dst_size, const char *src)
{
    size_t len;
    if (!dst || dst_size == 0) return;
    if (!src) { dst[0] = 0; return; }

    len = strlen(src);
    if (len >= dst_size) len = dst_size - 1;

    memcpy(dst, src, len);
    dst[len] = 0;
}

static u64 rgba(u8 r, u8 g, u8 b, u8 a)
{
    return GS_SETREG_RGBAQ(r, g, b, a, 0);
}

static void drawBox(float x1, float y1, float x2, float y2, u64 color)
{
    if (gsGlobal) {
        gsKit_prim_sprite(gsGlobal, x1, y1, x2, y2, 1, color);
    }
}

static void drawText(float x, float y, float scale, u64 color, const char *text)
{
    if (gsGlobal && fontm && text) {
        gsKit_fontm_print_scaled(gsGlobal, fontm, x, y, 1, scale, color, (char *)text);
    }
}

static void parse_video_mode(const char *mode)
{
    if (!strcasecmp(mode, "480p")) videoModeVal = GS_MODE_DTV_480P;
    else if (!strcasecmp(mode, "576p")) videoModeVal = GS_MODE_DTV_576P;
    else if (!strcasecmp(mode, "720p")) videoModeVal = GS_MODE_DTV_720P;
    else if (!strcasecmp(mode, "1080i")) videoModeVal = GS_MODE_DTV_1080I;
    else if (!strcasecmp(mode, "PAL")) videoModeVal = GS_MODE_PAL;
    else videoModeVal = GS_MODE_NTSC;
}

static int load_config_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    char line[256];
    if (!fp) return 0;

    while (fgets(line, sizeof(line), fp))
    {
        char *eq, *key, *value;
        size_t len;

        line[strcspn(line, "\r\n")] = 0;
        eq = strchr(line, '=');
        if (!eq) continue;

        *eq = 0;
        key = line;
        value = eq + 1;
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;

        len = strlen(key);
        while (len > 0 && (key[len - 1] == ' ' || key[len - 1] == '\t')) key[--len] = 0;

        if (!strcmp(key, "SITE_URL")) {
            copy_string(targetUrl, sizeof(targetUrl), value);
        } else if (!strcmp(key, "STORE_NAME")) {
            copy_string(storeName, sizeof(storeName), value);
        } else if (!strcmp(key, "VIDEO_MODE")) {
            copy_string(videoModeStr, sizeof(videoModeStr), value);
            parse_video_mode(value);
        }
    }

    fclose(fp);
    return 1;
}

static void load_config(void)
{
    if (load_config_file("mass:/SITEPS2/CONFIG.CFG")) return;
    load_config_file("mc0:/SITEPS2/CONFIG.CFG");
}

static void init_pad(void)
{
    int state;
    padInit(0);
    memset(padArea, 0, sizeof(padArea));

    if (!padPortOpen(PAD_PORT, PAD_SLOT, padArea)) {
        snprintf(statusLine, sizeof(statusLine), "Controle nao detectado.");
        return;
    }

    for (int i = 0; i < 20; i++) {
        state = padGetState(PAD_PORT, PAD_SLOT);
        if (state == PAD_STATE_STABLE || state == PAD_STATE_DISCONN) break;
        DelayThread(1000);
    }
}

static u32 read_pad_pressed(void)
{
    static u32 oldPad = 0;
    struct padButtonStatus buttons;
    u32 padData = 0, pressed = 0;

    if (padRead(PAD_PORT, PAD_SLOT, &buttons)) {
        padData = 0xffff ^ buttons.btns;
        pressed = padData & ~oldPad;
        oldPad = padData;
    }

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

    // Top Bar (Branca com linha divisoria cinza)
    drawBox(0, 0, w, 50, topBar);
    drawBox(0, 49, w, 50, border);
    drawText(20, 12, 0.75f, textDark, storeName);
    drawText(w - 140, 16, 0.50f, accentBlue, "[ WEB MODE ]");

    // Container Central
    drawBox(20, 70, w - 20, h - 50, cardBg);
    drawBox(20, 70, w - 20, 71, border);
    drawBox(20, h - 50, w - 20, h - 49, border);

    // Simbolo/Icone de Web (Desenhado via Sprite)
    drawBox(40, 90, 56, 106, accentBlue);
    drawText(65, 90, 0.65f, textDark, "ENDERECO CONFIGURADO:");

    // Input Box do Link (Estilo caixa de navegacao)
    drawBox(40, 120, w - 40, 160, bg);
    drawBox(40, 120, w - 40, 121, border);
    drawBox(40, 159, w - 40, 160, border);
    drawText(50, 132, 0.55f, textDark, targetUrl);

    // Botoes de Acao Simulado
    drawBox(40, 180, 180, 215, accentBlue);
    drawText(60, 190, 0.50f, topBar, "(X) CONECTAR");

    drawBox(190, 180, 310, 215, border);
    drawText(205, 190, 0.50f, textDark, "(O) SAIR");

    // Informacoes do Sistema
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
    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(255, 255, 255, 0));

    fontm = gsKit_init_fontm();
    if (fontm) {
        gsKit_fontm_upload(gsGlobal, fontm);
        gsKit_fontm_unpack(fontm);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int ret;
    (void)argc; (void)argv;

    SifInitRpc(0);

    ret = init_graphics();
    if (ret < 0) {
        SleepThread();
        return 1;
    }

    load_config();
    init_pad();

    for (;;) {
        handle_input();
        render();
    }

    return 0;
}
