#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <lvgl.h>

// Pinos do Display
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Buffer do LVGL
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

// Variáveis globais dos elementos da UI
lv_obj_t * arc_cpu;
lv_obj_t * label_cpu;
lv_obj_t * arc_ram;
lv_obj_t * label_ram;
lv_obj_t * table; 

// --- Função que o LVGL usa para "imprimir" na tela ---
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.drawRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp_drv);
}

// --- Função para construir a Interface ---
void build_ui() {
  lv_obj_t * tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 40);

  lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Gauges");
  lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Processos");

  // ABA 1: GAUGES
  lv_obj_t * cont = lv_obj_create(tab1);
  lv_obj_set_size(cont, 300, 160);
  lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW); 
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_opa(cont, 0, 0);

  arc_cpu = lv_arc_create(cont);
  lv_obj_set_size(arc_cpu, 110, 110);
  lv_arc_set_rotation(arc_cpu, 135);
  lv_arc_set_bg_angles(arc_cpu, 0, 270);
  lv_arc_set_value(arc_cpu, 0); 
  lv_obj_remove_style(arc_cpu, NULL, LV_PART_KNOB); 
  lv_obj_clear_flag(arc_cpu, LV_OBJ_FLAG_CLICKABLE); 

  label_cpu = lv_label_create(arc_cpu);
  lv_label_set_text(label_cpu, "--%\nCPU");
  lv_obj_align(label_cpu, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_align(label_cpu, LV_TEXT_ALIGN_CENTER, 0);

  arc_ram = lv_arc_create(cont);
  lv_obj_set_size(arc_ram, 110, 110);
  lv_arc_set_rotation(arc_ram, 135);
  lv_arc_set_bg_angles(arc_ram, 0, 270);
  lv_arc_set_value(arc_ram, 0); 
  lv_obj_set_style_arc_color(arc_ram, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR); 
  lv_obj_remove_style(arc_ram, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(arc_ram, LV_OBJ_FLAG_CLICKABLE);

  label_ram = lv_label_create(arc_ram);
  lv_label_set_text(label_ram, "--%\nRAM");
  lv_obj_align(label_ram, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_align(label_ram, LV_TEXT_ALIGN_CENTER, 0);

  // ABA 2: TABELA
  table = lv_table_create(tab2);
  lv_obj_set_width(table, 280);
  lv_obj_align(table, LV_ALIGN_CENTER, 0, 0);
  
  lv_table_set_col_width(table, 0, 180); 
  lv_table_set_col_width(table, 1, 100); 

  lv_table_set_cell_value(table, 0, 0, "Processo");
  lv_table_set_cell_value(table, 0, 1, "RAM (MB)");
  
  for(int i = 1; i <= 5; i++) {
    lv_table_set_cell_value(table, i, 0, "Aguardando...");
    lv_table_set_cell_value(table, i, 1, "-");
  }
}

// --- Função para processar os dados recebidos via Serial ---
void atualizarInterface(String dados) {
  int idxC = dados.indexOf("C:");
  int idxC_fim = dados.indexOf(";", idxC);
  if (idxC != -1 && idxC_fim != -1) {
    int cpu = dados.substring(idxC + 2, idxC_fim).toInt();
    lv_arc_set_value(arc_cpu, cpu);
    lv_label_set_text_fmt(label_cpu, "%d%%\nCPU", cpu);
  }

  int idxR = dados.indexOf("R:");
  int idxR_fim = dados.indexOf(";", idxR);
  if (idxR != -1 && idxR_fim != -1) {
    int ram = dados.substring(idxR + 2, idxR_fim).toInt();
    lv_arc_set_value(arc_ram, ram);
    lv_label_set_text_fmt(label_ram, "%d%%\nRAM", ram);
  }

  int idxP = dados.indexOf("P:");
  int idxP_fim = dados.lastIndexOf(";");
  if (idxP != -1 && idxP_fim != -1 && idxP_fim > idxP) {
    String processosStr = dados.substring(idxP + 2, idxP_fim);
    
    int inicioBusca = 0;
    int linha = 1;
    
    while (linha <= 5 && inicioBusca < processosStr.length()) {
      int virgula1 = processosStr.indexOf(',', inicioBusca);
      if (virgula1 == -1) break;
      String nomeProc = processosStr.substring(inicioBusca, virgula1);
      
      int virgula2 = processosStr.indexOf(',', virgula1 + 1);
      if (virgula2 == -1) virgula2 = processosStr.length();
      String ramProc = processosStr.substring(virgula1 + 1, virgula2);
      
      lv_table_set_cell_value(table, linha, 0, nomeProc.c_str());
      lv_table_set_cell_value(table, linha, 1, ramProc.c_str());
      
      inicioBusca = virgula2 + 1;
      linha++;
    }
    
    while (linha <= 5) {
      lv_table_set_cell_value(table, linha, 0, "");
      lv_table_set_cell_value(table, linha, 1, "");
      linha++;
    }
  }
}

void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1); 

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  build_ui();
}

void loop() {
  lv_timer_handler(); 
  delay(1); 
  lv_tick_inc(1); 

  if (Serial.available() > 0) {
    String dadosRecebidos = Serial.readStringUntil('\n');
    atualizarInterface(dadosRecebidos);
  }
}