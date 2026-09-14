/*
 * raspi-lcd-addon - entry point paket.
 *
 * File ini hanya meneruskan hasil build native binding, supaya bisa dipakai
 * dari project lain:
 *
 *   const { LM6029LCD } = require('raspi-lcd-addon');
 *   const lcd = new LM6029LCD();
 *   lcd.begin();            // opsional: lcd.begin(32000000) untuk SPI 32 MHz
 *
 * Tidak ada kode yang menyentuh hardware di sini. Semua contoh pemakaian
 * ada di folder examples/ (lihat examples/README.md).
 */

'use strict';

module.exports = require('./build/Release/lcd_lm6029.node');

