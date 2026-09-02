#include <stdio.h>
#include <string.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <gsInline.h>

// Definição de Cores Auxiliar
#define rgba(r, g, b, a) GS_SETREG_RGBAQ(r, g, b, a, 0x00)

// Ponteiros Globais do gsKit
static GSGLOBAL *gsGlobal = NULL;
static GSFONT *fontm = NULL;

// Variáveis Globais de Controle e Estado
static u32 oldPad = 0;
static char statusLine[128] = "Pronto.";
static char targetUrl[128] = "http://siteps2.local";
static char storeName[64] = "LOJA PS2 WEB";
static char videoModeStr[32] = "AUTO (NTSC/PAL)";
static int videoModeVal = GS_MODE_NTSC;

// Protótipos de Funções Não Definidas Neste Trecho
void load_config(void) {}
void init_pad(void) {}
u32 read_pad_raw(void) { return 0; }

// Função para Desenhar Caixas/Retângulos Usando gsKit NATIVO
static void drawBox(float x1, float y1, float x2, float y2, u64 color)
{
    gsKit_prim_sprite_color(gsGlobal, x1, y1, x2, y2, 1, color);
}

// Função para Desenhar Texto Usando gsKit NATIVO
static void drawText(float x, float y, float scale, u64 color, const char *text)
{
    if (fontm != NULL) {
        gsKit_fontm_print_scaled(gsGlobal, fontm, x, y, 1, scale, color, text);
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

    // Palette Light Mode
    u64 bg = rgba(245, 247, 250, 255);
    u64 topBar = rgba(255, 255, 255, 255);
    u64 border = rgba(210, 215, 225, 255);
    u64 cardBg = rgba(255, 255, 255, 255);
    u64 textDark = rgba(20, 24, 33, 255);
    u64 textMuted = rgba(100, 110, 125, 255);
    u64 accentBlue = rgba(24, 119, 242, 255);
    u64 accentGreen = rgba(34, 197, 94, 255);

    gsKit_clear(gsGlobal, bg);

    // Top Bar
    drawBox(0, 0, w, 50, topBar);
    drawBox(0, 49, w, 50, border);
    drawText(20, 12, 0.75f, textDark, storeName);
    drawText(w - 140, 16, 0.50f, accentBlue, "[ WEB MODE ]");

    // Container Central
    drawBox(20, 70, w - 20, h - 50, cardBg);
    drawBox(20, 70, w - 20, 71, border);
    drawBox(20, h - 50, w - 20, h - 49, border);

    // Ícone de Web
    drawBox(40, 90, 56, 106, accentBlue);
    drawText(65, 90, 0.65f, textDark, "ENDERECO CONFIGURADO:");

    // Input Box do Link
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
    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(255, 255, 255, 0));

    fontm = gsKit_init_fontm(GSKIT_FTYPE_FONTM);
    if (fontm) {
        gsKit_fontm_upload(gsGlobal, fontm);
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
