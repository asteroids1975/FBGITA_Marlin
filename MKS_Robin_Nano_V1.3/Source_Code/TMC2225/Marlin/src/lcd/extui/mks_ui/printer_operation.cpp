/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "draw_ui.h"
#include <lv_conf.h>

#include "../../../gcode/gcode.h"
#include "../../../module/planner.h"
#include "../../../module/motion.h"
#include "../../../sd/cardreader.h"
#include "../../../inc/MarlinConfig.h"
#include "../../../MarlinCore.h"
#include "../../../gcode/queue.h"

#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../../../feature/powerloss.h"
#endif

#if DISABLED(ADVANCED_PAUSE_FEATURE)
  #define PAUSE_PARK_RETRACT_FEEDRATE         25  // (mm/s) Initial retract feedrate.
  #define PAUSE_PARK_RETRACT_LENGTH          6.5  // (mm) Initial retract.
//  #define PARK_HEAD_ON_PAUSE                    // Park the nozzle during pause and filament change.  
#endif
#if DISABLED(NOZZLE_PARK_FEATURE)
//  #define NOZZLE_PARK_POINT { (X_MIN_POS + 5), (Y_MIN_POS + 5), 5 }
  #define NOZZLE_PARK_XY_FEEDRATE 100   // (mm/s) X and Y axes feedrate (also used for delta Z axis)
#endif


extern uint32_t To_pre_view;
extern bool flash_preview_begin, default_preview_flg, gcode_preview_over;

void printer_state_polling() {
  char str_1[16];
  char str_2[16];
  if (uiCfg.print_state == PAUSING) {
    #if ENABLED(SDSUPPORT)
      if (!planner.has_blocks_queued() && card.getIndex() > MIN_FILE_PRINTED)
        uiCfg.waitEndMoves++;

      if (uiCfg.waitEndMoves > 20) {
        uiCfg.waitEndMoves = 0;
        planner.synchronize();

        gcode.process_subcommands_now(F("M25"));

        // save the position
        uiCfg.current_x_position_bak = current_position.x;
        uiCfg.current_y_position_bak = current_position.y;
        uiCfg.current_z_position_bak = current_position.z;
        uiCfg.current_e_position_bak = current_position.e;
  #if ENABLED(PARK_HEAD_ON_PAUSE)        
        sprintf_P(public_buf_m, PSTR("G92 E0\nG1 F%d E%s\n"),PAUSE_PARK_RETRACT_FEEDRATE*60,dtostrf(-PAUSE_PARK_RETRACT_LENGTH, 1, 3, str_1));
        gcode.process_subcommands_now(public_buf_m);

        if (gCfgItems.pausePosZ != (float)-1) {
          sprintf_P(public_buf_m, PSTR("G91\nG1 Z%s\nG90\n"), dtostrf(gCfgItems.pausePosZ, 1, 3, str_1));
          gcode.process_subcommands_now(public_buf_m);
        }
        if (gCfgItems.pausePosX != (float)-1 && gCfgItems.pausePosY != (float)-1) {
          //sprintf_P(public_buf_l, PSTR("G1 X%s Y%s"), dtostrf(gCfgItems.pausePosX, 1, 1, str_1), dtostrf(gCfgItems.pausePosY, 1, 1, str_1));
          sprintf_P(public_buf_m, PSTR("G1 F%d X%s Y%s\n"),NOZZLE_PARK_XY_FEEDRATE*60, dtostrf(gCfgItems.pausePosX, 1, 3, str_1), dtostrf(gCfgItems.pausePosY, 1, 3, str_2));
          gcode.process_subcommands_now(public_buf_m);
        }
  #endif
        uiCfg.print_state = PAUSED;

        gCfgItems.pause_reprint = true;
        update_spi_flash();
      }
    #endif
  }
  else
    uiCfg.waitEndMoves = 0;

  if (uiCfg.print_state == PAUSED) {
  }

  if (uiCfg.print_state == RESUMING) {
    if (IS_SD_PAUSED()) {

  #if ENABLED(PARK_HEAD_ON_PAUSE)      
        //ZERO(public_buf_m);
        if (gCfgItems.pausePosX != (float)-1 && gCfgItems.pausePosY != (float)-1) {
          sprintf_P(public_buf_m, PSTR("G90"));
          gcode.process_subcommands_now(public_buf_m);
          sprintf_P(public_buf_m, PSTR("G1 F%d X%s Y%s\n"),NOZZLE_PARK_XY_FEEDRATE*60, dtostrf(uiCfg.current_x_position_bak, 1, 3, str_1), dtostrf(uiCfg.current_y_position_bak, 1, 3, str_2));
          gcode.process_subcommands_now(public_buf_m);
        }
        if (gCfgItems.pausePosZ != (float)-1) {
          sprintf_P(public_buf_m, PSTR("G1 F%d Z%s"), PAUSE_PARK_RETRACT_FEEDRATE*60, dtostrf(uiCfg.current_z_position_bak, 1, 3, str_1));
          gcode.process_subcommands_now(public_buf_m);
        }
        sprintf_P(public_buf_m, PSTR("G92 E%s"), dtostrf(uiCfg.current_e_position_bak - PAUSE_PARK_RETRACT_LENGTH, 1, 5, str_1));
        gcode.process_subcommands_now(public_buf_m);

        sprintf_P(public_buf_m, PSTR("G1 F%d E%s"), PAUSE_PARK_RETRACT_FEEDRATE*60 ,dtostrf(uiCfg.current_e_position_bak, 1, 5, str_2));
        gcode.process_subcommands_now(public_buf_m);
        planner.synchronize();        
  #endif

      gcode.process_subcommands_now(FPSTR(M24_STR));
        /*
        if (card.isFileOpen()) {
          card.startOrResumeFilePrinting();            // SD card will now be read for commands
          startOrResumeJob();               // Start (or resume) the print job timer
          TERN_(POWER_LOSS_RECOVERY, recovery.prepare());
        }      
        */
      uiCfg.print_state = WORKING;
      start_print_time();

      gCfgItems.pause_reprint = false;
      update_spi_flash();
    }
  }
  #if ENABLED(POWER_LOSS_RECOVERY)
    if (uiCfg.print_state == REPRINTED) {
      send_str_to_wifi("; printer_state_polling: REPRINTING!\r\n");
#if 0
      #if HAS_HEATED_BED
        const int16_t bt = recovery.info.target_temperature_bed;
        sprintf_P(public_buf_m, PSTR("M140 S%i"), bt);
        gcode.process_subcommands_now(public_buf_m);
      #endif
      #if HAS_HOTEND
        HOTEND_LOOP() {
          const int16_t et = recovery.info.target_temperature[e];
          if (et) {
            #if HAS_MULTI_HOTEND
              sprintf_P(public_buf_m, PSTR("T%i"), e);
              gcode.process_subcommands_now(public_buf_m);
            #endif
            sprintf_P(public_buf_m, PSTR("M109 S%i"), et);
            gcode.process_subcommands_now(public_buf_m);
          }
        }
      #endif
#endif

      recovery.resume();
      #if 0
        // Move back to the saved XY
        char str_1[16], str_2[16];
        sprintf_P(public_buf_m, PSTR("G1 X%s Y%s F2000"),
          dtostrf(recovery.info.current_position.x, 1, 3, str_1),
          dtostrf(recovery.info.current_position.y, 1, 3, str_2)
        );
        gcode.process_subcommands_now(public_buf_m);

        if (gCfgItems.pause_reprint && gCfgItems.pausePosZ != -1.0f) {
          sprintf_P(public_buf_l, PSTR("G91\nG1 Z-%s\nG90"), dtostrf(gCfgItems.pausePosZ, 1, 1, str_2));
          gcode.process_subcommands_now(public_buf_l);
        }
      #endif
      uiCfg.print_state = WORKING;
      start_print_time();

      gCfgItems.pause_reprint = false;
      update_spi_flash();
    }
  #endif

  if (uiCfg.print_state == WORKING)
    filament_check();

  TERN_(MKS_WIFI_MODULE, wifi_looping());
}

void filament_pin_setup() {
  #if PIN_EXISTS(MT_DET_1)
    SET_INPUT_PULLUP(MT_DET_1_PIN);
  #endif
  #if PIN_EXISTS(MT_DET_2)
    SET_INPUT_PULLUP(MT_DET_2_PIN);
  #endif
  #if PIN_EXISTS(MT_DET_3)
    SET_INPUT_PULLUP(MT_DET_3_PIN);
  #endif
}

void filament_check() {
  #if ANY_PIN(MT_DET_1, MT_DET_2, MT_DET_3)
    const int FIL_DELAY = 20;
  #endif
  #if PIN_EXISTS(MT_DET_1)
    static int fil_det_count_1 = 0;
    if (READ(MT_DET_1_PIN) == MT_DET_PIN_STATE)
      fil_det_count_1++;
    else if (fil_det_count_1 > 0)
      fil_det_count_1--;
  #endif

  #if PIN_EXISTS(MT_DET_2)
    static int fil_det_count_2 = 0;
    if (READ(MT_DET_2_PIN) == MT_DET_PIN_STATE)
      fil_det_count_2++;
    else if (fil_det_count_2 > 0)
      fil_det_count_2--;
  #endif

  #if PIN_EXISTS(MT_DET_3)
    static int fil_det_count_3 = 0;
    if (READ(MT_DET_3_PIN) == MT_DET_PIN_STATE)
      fil_det_count_3++;
    else if (fil_det_count_3 > 0)
      fil_det_count_3--;
  #endif

  if (false
    #if PIN_EXISTS(MT_DET_1)
      || fil_det_count_1 >= FIL_DELAY
    #endif
    #if PIN_EXISTS(MT_DET_2)
      || fil_det_count_2 >= FIL_DELAY
    #endif
    #if PIN_EXISTS(MT_DET_3)
      || fil_det_count_3 >= FIL_DELAY
    #endif
  ) {
    send_str_to_wifi("; detected filament runout!\r\n");
    clear_cur_ui();
    card.pauseSDPrint();
    stop_print_time();
    uiCfg.print_state = PAUSING;

    if (gCfgItems.from_flash_pic)
      flash_preview_begin = true;
    else
      default_preview_flg = true;

    lv_draw_printing();
  }
}

#endif // HAS_TFT_LVGL_UI
