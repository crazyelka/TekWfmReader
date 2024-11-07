#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include "wfm.h"

struct Wfm *map_wfm(const char *path) {
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
    file.read(reinterpret_cast<char*>(wfm->Hdr), sizeof(WfmHdr));

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

    // curve buffer
    wfm->CurveBuffer = new uint8_t[wfm->Static->BytesRemaining];
    file.seekg(wfm->Static->CurveBufferOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(wfm->CurveBuffer), wfm->Static->BytesRemaining);

    wfm->Checksum = 0;
    wfm->Size = static_cast<uint64_t>(file.tellg());

    file.close();
    return wfm;
}

int unmap_wfm(Wfm *wfm) {
    delete[] wfm->CurveBuffer;
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

void print_wfm_info(const Wfm *wfm) {
    if (!wfm) {
        std::cerr << "Invalid Wfm." << std::endl;
        return;
    }

    // Print static file info
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

    // Print header info
    std::cout << "\nHeader Info:" << std::endl;
    std::cout << "Set Type: " << wfm->Hdr->Ref.SetType << std::endl;
    std::cout << "Waveform Count: " << wfm->Hdr->Ref.WfmCnt << std::endl;
    std::cout << "Data Type: " << wfm->Hdr->Ref.DataType << std::endl;
    std::cout << "Voltage Scale: " << wfm->Hdr->ExpDim1.Scale << std::endl;
    std::cout << "Voltage Offset: " << wfm->Hdr->ExpDim1.Offset << std::endl;
    std::cout << "Time Scale: " << wfm->Hdr->ImpDim1.Scale << std::endl;
    std::cout << "Time Offset: " << wfm->Hdr->ImpDim1.Offset << std::endl;

    // Print other relevant information as needed...
}

void print_curve_data(const Wfm *wfm) {
    if (!wfm || !wfm->CurveBuffer) {
        std::cerr << "Invalid Wfm or CurveBuffer is null." << std::endl;
        return;
    }

    // Output curve data in a table format
    size_t curve_size = (wfm->Static->NFastFrames + 1) * wfm->Hdr->CurveSpec.PostchargeStop;
    std::cout << "\nCurve Data (first 10 points):" << std::endl;
    std::cout << std::setw(10) << "Index" << std::setw(15) << "Raw Data" << std::endl;
    std::cout << std::string(25, '-') << std::endl;

    for (size_t i = 0; i < std::min(curve_size, static_cast<size_t>(10)); i++)
    {
        std::cout << std::setw(10) << i << std::setw(15) << static_cast<int>(wfm->CurveBuffer[i]) << std::endl;
    }
}

void calculate_and_print_voltages(const Wfm *wfm) {
    if (!wfm) {
        std::cerr << "Invalid Wfm." << std::endl;
        return;
    }

    size_t curve_size = (wfm->Static->NFastFrames + 1) * wfm->Hdr->CurveSpec.PostchargeStop;
    int16_t *curve_data = reinterpret_cast<int16_t *>(wfm->CurveBuffer + wfm->Hdr->CurveSpec.DataStart);

    std::cout << "\nCalculated Voltages:" << std::endl;
    std::cout << std::setw(10) << "Point" << std::setw(15) << "Voltage (V)" << std::endl;
    std::cout << std::string(25, '-') << std::endl;

    for (size_t i = 0; i < curve_size / sizeof(int16_t); i++) {
        double voltage = (curve_data[i] * wfm->Hdr->ExpDim1.Scale) + wfm->Hdr->ExpDim1.Offset;
        std::cout << std::setw(10) << i << std::setw(15) << voltage << std::endl;
    }
}
