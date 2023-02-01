/*****************************************************************************

Copyright (c) 2012-2022  Michal Necasek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*****************************************************************************/

/* Mode table support. */

#include "boxv.h"       /* Public interface. */
#include "boxvint.h"    /* Implementation internals. */
#include "boxv_io.h"    /* I/O access layer, host specific. */


v_vgaregs       vga_regs_ext = {
    0xe3, { 0x01, 0x01, 0x0f, 0x00, 0x0a }, {
    0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
    0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xea, 0x0c, 0xdf, 0x28, 0x4f, 0xe7, 0x04, 0xe3, 0xff }, {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff }, {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x41, 0x00, 0x0f, 0x00, 0x00 }
};

v_mode          mode_640_480_8 = {
    0x101, 640, 480, 8, 1, &vga_regs_ext
};

v_mode          mode_800_600_8 = {
    0x103, 800, 600, 8, 1, &vga_regs_ext
};

v_mode          mode_1024_768_8 = {
    0x105, 1024, 768, 8, 1, &vga_regs_ext
};

v_mode          mode_640_480_16 = {
    0x111, 640, 480, 16, 1, &vga_regs_ext
};

v_mode          mode_800_600_16 = {
    0x114, 800, 600, 16, 1, &vga_regs_ext
};

v_mode          mode_1024_768_16 = {
    0x117, 1024, 768, 16, 1, &vga_regs_ext
};

v_mode          mode_640_480_32 = {
    0x129, 640, 480, 32, 1, &vga_regs_ext
};

v_mode          mode_800_600_32 = {
    0x12E, 800, 600, 32, 1, &vga_regs_ext
};

v_mode          mode_1024_768_32 = {
    0x138, 1024, 768, 32, 1, &vga_regs_ext
};

v_mode          *mode_list[] = {
    &mode_640_480_8,
    &mode_800_600_8,
    &mode_1024_768_8,
    &mode_640_480_16,
    &mode_800_600_16,
    &mode_1024_768_16,
    &mode_640_480_32,
    &mode_800_600_32,
    &mode_1024_768_32,
    NULL
};

/* Write a single value to an indexed register at a specified
 * index. Suitable for the CRTC or graphics controller.
 */
inline void vid_wridx( void *cx, int idx_reg, int idx, v_byte data )
{
    vid_outw( cx, idx_reg, idx | (data << 8) );
}

/* Program a sequence of bytes into an indexed register, starting
 * at index 0. Suitable for loading the CRTC or graphics controller.
 */
static void vid_wridx_s( void *cx, int idx_reg, int count, v_byte *data )
{
    int     idx;

    for( idx = 0; idx < count; ++idx )
        vid_wridx( cx, idx_reg, idx, data[idx] );   /* Write index/data. */
}

/* Program a sequence of bytes into the attribute controller, starting
 * at index 0. Note: This function may not be interrupted by code which
 * also accesses the attribute controller.
 */
static void vid_wratc_s( void *cx, int count, v_byte *data )
{
    int     idx;

    vid_inb( cx, VGA_STAT_ADDR );               /* Reset flip-flop. */
    for( idx = 0; idx < count; ++idx ) {
        vid_outb( cx, VGA_ATTR_W, idx );        /* Write index. */
        vid_outb( cx, VGA_ATTR_W, data[idx] );  /* Write data. */
    }
}

v_mode *find_mode( int mode_no )
{
    v_mode      **p_mode;
    v_mode      *mode;

    mode = NULL;
    for( p_mode = mode_list; *p_mode; ++p_mode ) {
        if( (*p_mode)->mode_no == mode_no ) {
            mode = *p_mode;
            break;
        }
    }
    return( mode );
}

/* Enumerate all available modes. Runs a callback for each mode; if the
 * callback returns zero, the enumeration is terminated.
 */
void BOXV_mode_enumerate( void *cx, int (cb)( void *cx, BOXV_mode_t *mode ) )
{
    BOXV_mode_t     mode_info;
    v_mode          **p_mode;
    v_mode          *mode;

    for( p_mode = mode_list; *p_mode; ++p_mode ) {
        mode = *p_mode;
        mode_info.mode_no = mode->mode_no;
        mode_info.xres    = mode->xres;
        mode_info.yres    = mode->yres;
        mode_info.bpp     = mode->bpp;
        if( !cb( cx, &mode_info ) )
            break;
    }
}

/* Set the requested mode (text or graphics).
 * Returns non-zero value on failure.
 */
int BOXV_mode_set( void *cx, int mode_no )
{
    v_mode          *mode;
    v_vgaregs       *vgarg;

    mode = find_mode( mode_no );
    if( !mode )
        return( -1 );

    /* Put the hardware into a state where the mode can be safely set. */
    vid_inb( cx, VGA_STAT_ADDR );                   /* Reset flip-flop. */
    vid_outb( cx, VGA_ATTR_W, 0 );                  /* Disable palette. */
    vid_wridx( cx, VGA_CRTC, VGA_CR_VSYNC_END, 0 ); /* Unlock CR0-CR7. */
    vid_wridx( cx, VGA_SEQUENCER, VGA_SR_RESET, VGA_SR_RESET );

    /* Disable the extended display registers. */
    vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE );
    vid_outw( cx, VBE_DISPI_IOPORT_DATA, VBE_DISPI_DISABLED );

    /* Optionally program the extended non-VGA registers. */
    if( mode->ext ) {
        /* Set X resoultion. */
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, mode->xres );
        /* Set Y resoultion. */
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, mode->yres );
        /* Set bits per pixel. */
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, mode->bpp );
        /* Set the virtual resolution. */
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_WIDTH );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, mode->xres );
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_HEIGHT );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, mode->yres );
        /* Reset the current bank. */
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BANK );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, 0 );
        /* Set the X and Y display offset to 0. */
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_X_OFFSET );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, 0 );
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_Y_OFFSET );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, 0 );
        /* Enable the extended display registers. */
        vid_outw( cx, VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE );
        vid_outw( cx, VBE_DISPI_IOPORT_DATA, VBE_DISPI_ENABLED | VBE_DISPI_8BIT_DAC );
    }

    vgarg = mode->vgaregs;

    /* Program misc. output register. */
    vid_outb( cx, VGA_MISC_OUT_W, vgarg->misc );

    /* Program the sequencer. */
    vid_wridx_s( cx, VGA_SEQUENCER, sizeof( vgarg->seq ), vgarg->seq );
    vid_wridx( cx, VGA_SEQUENCER, VGA_SR_RESET, VGA_SR0_NORESET );

    /* Program the CRTC and graphics controller. */
    vid_wridx_s( cx, VGA_CRTC, sizeof( vgarg->crtc ), vgarg->crtc );
    vid_wridx_s( cx, VGA_GRAPH_CNTL, sizeof( vgarg->gctl ), vgarg->gctl );

    /* Finally program the attribute controller. */
    vid_wratc_s( cx, sizeof( vgarg->atr ), vgarg->atr );
    vid_outb( cx, VGA_ATTR_W, 0x20 );               /* Re-enable palette. */

    return( 0 );
}
