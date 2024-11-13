#ifndef WFM_H
#define WFM_H

#include <vector>
#include <iostream>
#include <stdint.h>

//typedef std::variant<int16_t, uint32_t, uint64_t, float, double, uint8_t, int8_t> buffTypes;

struct Wfm {
    struct WfmStaticFileInfo *Static;
    struct WfmHdr *Hdr;
    /* Optional */
    struct WfmFastFrames *FastFrames;
    /* Curve buffer [5]
     *
     * Contains curve data (inclusive of pre/post charge) for all
     * waveforms in set in a contiguous block (see notes below).
     *
     * len: (Static.NFastFrames + 1) * Hdr.CurveSpec.PostchargeStop
     */
    std::vector<int8_t> curvePreBuff;
    std::vector<int8_t> curveBuff;
    std::vector<int8_t> curvePostBuff;

    /* Waveform file checksum
     *
     * Checksum for the waveform file. The checksum is calculated by
     * summing all data values from the Waveform header through the
     * Curve data as unsigned chars.
     */
    uint64_t Checksum;
    /* Total size of waveform file in bytes */
    uint64_t Size;
};

/**
 * /func    map_wfm
 *
 * /brief   Function open WFM file and maps into memory
 *
 * /param   path    :   Path to the file
 *
 * /retval  Pointer to Wfm struct
 */
struct Wfm *map_wfm(const char *path);

/**
 * /func    unmap_wfm
 *
 * /brief   Function free the memory
 *
 * /param   wfm    :   Pointer to be freed
 *
 * /retval  int    :   Error code
 */
int unmap_wfm(struct Wfm *wfm);

/**
 * /func    print_wfm_info
 *
 * /brief   Function prints information about Wfm struct
 *
 * /param   wfm    :   Pointer to the Wfm
 *
 * /retval  void
 */
void print_wfm_info(const Wfm *wfm);

/**
 * /func    calculate_and_print_voltages
 *
 * /brief   Function calculate voltages and prints curve
 *
 * /param   wfm    :   Pointer to the Wfm
 *
 * /retval  void
 */
void calculate_and_print_voltages(const Wfm *wfm);

/**
 * /func    write_wfm
 *
 * /brief   Function writes Wfm struct in new file
 *
 * /param   filename      :   File name
 *          wfm_reference :   Wfm Struct for copy information
 *
 * /retval  void
 */
void write_wfm(const char *filename, const Wfm *wfm_reference);

/**
 * /func    write_wfm
 *
 * /brief   Function calculated by summing all data values from data as unsigned chars
 *
 * /param   data          :   Data
 *          size          :   Size
 *
 * /retval  uint64_t
 */
uint64_t sum_bytes(const uint8_t *data, size_t size);

/**
 * /func    calc_checksum
 *
 * /brief   Function calculate checksum for Wfm file
 *
 * /param   wfm      :   Wfm Struct for calc checksum
 *
 * /retval  uint64_t
 */
uint64_t calc_checksum(const Wfm *wfm);

#endif //WFM_H

