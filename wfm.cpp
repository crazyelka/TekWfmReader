#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <stdint.h>
#include <string>
#include <vector>
#include <variant>

#include "wfm_structs.h"
#include "wfm.h"

using namespace  std;

/**
 * /func    map_wfm
 *
 * /brief   Function open WFM file and maps into memory
 *
 * /param   path    :   Path to the file
 *
 * /retval  Pointer to Wfm struct
 */
struct Wfm *map_wfm(const char *path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open `" << path << "`" << std::endl;
        return nullptr;
    }

    Wfm *wfm = new Wfm;

    if (!wfm) {
        std::cerr << "Memory allocation failed." << std::endl;
        return nullptr;
    }

    // static file info
    wfm->Static = new WfmStaticFileInfo;

    file.read(reinterpret_cast<char*>(wfm->Static), sizeof(WfmStaticFileInfo));

    // header
    wfm->Hdr = new WfmHdr;
    file.read(reinterpret_cast<char*>(wfm->Hdr), (sizeof (WfmHdr)));

    // FastFrames if available
    if (wfm->Static->NFastFrames > 0) {
        wfm->FastFrames = new WfmFastFrames;
        wfm->FastFrames->UpdateSpecs = new WfmUpdateSpec[wfm->Static->NFastFrames];
        wfm->FastFrames->CurveSpecs = new WfmCurveSpec[wfm->Static->NFastFrames];
        file.read(reinterpret_cast<char*>(wfm->FastFrames->UpdateSpecs), sizeof(WfmUpdateSpec) * wfm->Static->NFastFrames);
        file.read(reinterpret_cast<char*>(wfm->FastFrames->CurveSpecs), sizeof(WfmCurveSpec) * wfm->Static->NFastFrames);
    } else {
        wfm->FastFrames = nullptr;
    }

    // curve buffers
    int size_precharge = wfm->Hdr->CurveSpec.PrechargeStart + wfm->Hdr->CurveSpec.DataStart;
    wfm->curvePreBuff.resize(size_precharge);
    file.read(reinterpret_cast<char*>(wfm->curvePreBuff.data()), size_precharge);

    int size_buff = wfm->Hdr->CurveSpec.PostchargeStart - wfm->Hdr->CurveSpec.DataStart;
    wfm->curveBuff.resize(size_buff);
    file.read(reinterpret_cast<char*>(wfm->curveBuff.data()), size_buff);

    int size_postcharge =  wfm->Hdr->CurveSpec.PostchargeStop - wfm->Hdr->CurveSpec.PostchargeStart;
    wfm->curvePostBuff.resize(size_postcharge);
    file.read(reinterpret_cast<char*>(wfm->curvePostBuff.data()), size_postcharge);

    // checksum
    file.read(reinterpret_cast<char*>(&wfm->Checksum), sizeof (wfm->Checksum));

    file.close();
    return wfm;
}

/**
 * /func    unmap_wfm
 *
 * /brief   Function free the memory
 *
 * /param   wfm    :   Pointer to be freed
 *
 * /retval  int    :   Error code
 */
int unmap_wfm(Wfm *wfm)
{
    if (wfm->FastFrames) {
        delete[] wfm->FastFrames->UpdateSpecs;
        delete[] wfm->FastFrames->CurveSpecs;
        delete wfm->FastFrames;
    }
    delete wfm->Hdr;
    delete wfm->Static;
    delete wfm;
    return 0;
}

/**
 * /func    print_wfm_info
 *
 * /brief   Function prints information about Wfm struct
 *
 * /param   wfm    :   Pointer to the Wfm
 *
 * /retval  void
 */
void print_wfm_info(const Wfm *wfm)
{
    if (!wfm) {
        std::cerr << "Invalid Wfm." << std::endl;
        return;
    }

    // static file info
    std::cout << "Static File Info:" << std::endl;
    std::cout << "Byte Order Verification: " << wfm->Static->ByteOrderVerification << std::endl;
    std::cout << "Version Number: " << wfm->Static->VersionNumber << std::endl;
    std::cout << "Byte Count Digits: " << static_cast<int>(wfm->Static->ByteCountDigits) << std::endl;
    std::cout << "Bytes Remaining: " << wfm->Static->BytesRemaining << std::endl;
    std::cout << "Bytes Per Point: " << static_cast<int>(wfm->Static->BytesPerPoint) << std::endl;
    std::cout << "Curve Buffer Offset: " << wfm->Static->CurveBufferOffset << std::endl;
    std::cout << "Horizontal Zoom Scale Factor: " << wfm->Static->HorizontalZoomScaleFactor << std::endl;
    std::cout << "Horizontal Zoom Position: " << wfm->Static->HorizontalZoomPosition << std::endl;
    std::cout << "Vertical Zoom Scale Factor: " << wfm->Static->VerticalZoomScaleFactor << std::endl;
    std::cout << "Vertical Zoom Position: " << wfm->Static->VerticalZoomPosition << std::endl;
    std::cout << "Label: " << wfm->Static->Label << std::endl;
    std::cout << "Number of FastFrames: " << wfm->Static->NFastFrames << std::endl;
    std::cout << "Header Size: " << wfm->Static->HdrSize << std::endl;

    // header info
    std::cout << "\nHeader Info:" << std::endl;
    std::cout << "Set Type: " << wfm->Hdr->Ref.SetType << std::endl;
    std::cout << "Waveform Count: " << wfm->Hdr->Ref.WfmCnt << std::endl;
    std::cout << "Data Type: " << wfm->Hdr->Ref.DataType << std::endl;
    std::cout << "Voltage Scale: " << wfm->Hdr->ExpDim1.Scale << std::endl;
    std::cout << "Voltage Offset: " << wfm->Hdr->ExpDim1.Offset << std::endl;
    std::cout << "Time Scale: " << wfm->Hdr->ImpDim1.Scale << std::endl;
    std::cout << "Time Offset: " << wfm->Hdr->ImpDim1.Offset << std::endl;

}

/**
 * /func    calculate_and_print_voltages
 *
 * /brief   Function calculate voltages and prints curve
 *
 * /param   wfm    :   Pointer to the Wfm
 *
 * /retval  void
 */
void calculate_and_print_voltages(const Wfm *wfm)
{
    if (!wfm) {
        std::cerr << "Invalid Wfm." << std::endl;
        return;
    }

    std::cout << "\nCalculated Voltages:" << std::endl;
    std::cout << std::setw(10) << "Point" << std::setw(15) << "Voltage (V)" << std::endl;
    std::cout << std::string(25, '-') << std::endl;

    for (size_t i = 0; i < wfm->curveBuff.size() ; i++) {
        double voltage = ((wfm->curveBuff[i]) * wfm->Hdr->ExpDim1.Scale) + wfm->Hdr->ExpDim1.Offset;
        if (i < 10 || i > 990)
            std::cout << std::setw(10) << i << std::setw(15) << voltage<< std::setw(15) << std::endl;
    }
}

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
void write_wfm(const char *filename, const Wfm *wfm_reference)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "Unable to open file for writing." << std::endl;
        return;
    }

    WfmStaticFileInfo *staticInfo = wfm_reference->Static;
    WfmHdr            *header     = wfm_reference->Hdr;
    uint64_t Checksum = calc_checksum(wfm_reference);

    file.write(reinterpret_cast<char*>(staticInfo), sizeof (WfmStaticFileInfo));
    file.write(reinterpret_cast<char*>(header), sizeof (WfmHdr));

    file.write(reinterpret_cast<const char*>(wfm_reference->curvePreBuff.data()), wfm_reference->curvePreBuff.size());
    file.write(reinterpret_cast<const char*>(wfm_reference->curveBuff.data()), wfm_reference->curveBuff.size());
    file.write(reinterpret_cast<const char*>(wfm_reference->curvePostBuff.data()), wfm_reference->curvePostBuff.size());

    file.write(reinterpret_cast<const char*>(&Checksum), sizeof (Checksum));

    file.close();

    std::cout << "Writing completed!" << std::endl;
}

/**
 * /func    calc_checksum
 *
 * /brief   Function calculate checksum for Wfm file
 *
 * /param   wfm      :   Wfm Struct for calc checksum
 *
 * /retval  uint64_t
 */
uint64_t calc_checksum(const Wfm *wfm)
{
    uint64_t checksum = 0;

    const WfmStaticFileInfo *staticFileInfo = wfm->Static;
    const WfmHdr *hdr = wfm->Hdr;

    checksum += sum_bytes(reinterpret_cast<const uint8_t *>(staticFileInfo), sizeof (WfmStaticFileInfo));
    checksum += sum_bytes(reinterpret_cast<const uint8_t *>(hdr), sizeof (WfmHdr));

    checksum += sum_bytes(reinterpret_cast<const uint8_t *>(wfm->curvePreBuff.data()), wfm->curvePreBuff.size());
    checksum += sum_bytes(reinterpret_cast<const uint8_t *>(wfm->curveBuff.data()), wfm->curveBuff.size());
    checksum += sum_bytes(reinterpret_cast<const uint8_t *>(wfm->curvePostBuff.data()), wfm->curvePostBuff.size());

    return checksum;
}

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
uint64_t sum_bytes(const uint8_t *data, size_t size)
{
    uint64_t checksum = 0;

    for (size_t i = 0; i < size; i++)
    {
        checksum += static_cast<uint64_t>(data[i]);
    }
    return checksum;
}
