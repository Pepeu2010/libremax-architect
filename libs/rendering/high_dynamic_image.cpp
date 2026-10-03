#include "high_dynamic_image.h"
#include <FreeImage.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfIO.h>
#include <OpenEXR/ImfInputFile.h>
#include <QtEndian>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>
namespace lmx {
namespace {
using Memory = std::unique_ptr<FIMEMORY, decltype(&FreeImage_CloseMemory)>;
using Bitmap = std::unique_ptr<FIBITMAP, decltype(&FreeImage_Unload)>;
class ExrMemory final : public Imf::IStream {
    const QByteArray &bytes;
    std::uint64_t offset = 0;

  public:
    explicit ExrMemory(const QByteArray &data) : Imf::IStream("embedded.exr"), bytes(data) {}
    bool read(char *destination, int count) override {
        if (count < 0 || offset > static_cast<std::uint64_t>(bytes.size()) ||
            static_cast<std::uint64_t>(count) > static_cast<std::uint64_t>(bytes.size()) - offset)
            throw std::invalid_argument("Imagem EXR truncada");
        std::memcpy(destination, bytes.constData() + offset, count);
        offset += count;
        return offset < static_cast<std::uint64_t>(bytes.size());
    }
    std::uint64_t tellg() override { return offset; }
    void seekg(std::uint64_t position) override {
        if (position > static_cast<std::uint64_t>(bytes.size()))
            throw std::invalid_argument("Posição EXR inválida");
        offset = position;
    }
};
void initialize() {
    static const auto ready = [] {
        FreeImage_Initialise();
        return true;
    }();
    (void)ready;
}
QString field(const QByteArray &bytes, qsizetype &offset) {
    const auto end = bytes.indexOf('\0', offset);
    if (end < offset || end - offset > 255)
        throw std::invalid_argument("Cabeçalho EXR inválido");
    const auto result = QString::fromLatin1(bytes.mid(offset, end - offset));
    offset = end + 1;
    return result;
}
QSize exrSize(const QByteArray &bytes) {
    if (bytes.size() < 9 || qFromLittleEndian<quint32>(bytes.constData()) != 20000630)
        throw std::invalid_argument("Arquivo EXR inválido");
    const auto version = qFromLittleEndian<quint32>(bytes.constData() + 4);
    if ((version & 255) != 2 || (version & (0x800 | 0x1000)))
        throw std::invalid_argument("Use um EXR de imagem única, sem dados deep");
    qsizetype offset = 8;
    QSize size;
    while (offset < bytes.size() && offset < 1024 * 1024) {
        const auto name = field(bytes, offset);
        if (name.isEmpty())
            return size;
        const auto type = field(bytes, offset);
        if (offset + 4 > bytes.size())
            break;
        const auto length = qFromLittleEndian<quint32>(bytes.constData() + offset);
        offset += 4;
        if (length > 1024 * 1024 || length > bytes.size() - offset)
            break;
        if (name == "dataWindow") {
            if (type != "box2i" || length != 16 || size.isValid())
                break;
            auto value = [&](int index) {
                return qFromLittleEndian<qint32>(bytes.constData() + offset + index * 4);
            };
            const qint64 width = qint64(value(2)) - value(0) + 1, height = qint64(value(3)) - value(1) + 1;
            if (width < 1 || height < 1 || width > 8192 || height > 8192 ||
                std::abs(qint64(value(0))) > 8192 || std::abs(qint64(value(1))) > 8192)
                break;
            size = QSize(int(width), int(height));
        }
        offset += length;
    }
    throw std::invalid_argument("Cabeçalho ou dimensões EXR inválidos");
}
} // namespace
HighDynamicImage inspectHighDynamicImage(const QByteArray &bytes, bool decode, qint64 maximumPixels) {
    if (bytes.isEmpty() || bytes.size() > 512 * 1024 * 1024)
        throw std::invalid_argument("Imagem de luz ausente ou grande demais");
    const bool exr = bytes.size() >= 4 && qFromLittleEndian<quint32>(bytes.constData()) == 20000630;
    if (exr) {
        HighDynamicImage result{"exr", exrSize(bytes), 0};
        if (!result.size.isValid() || qint64(result.size.width()) * result.size.height() > maximumPixels)
            throw std::invalid_argument("Imagem EXR grande demais");
        if (!decode)
            return result;
        try {
            ExrMemory stream(bytes);
            Imf::InputFile input(stream, 1);
            const auto window = input.header().dataWindow();
            if (!input.isComplete() ||
                QSize(window.max.x - window.min.x + 1, window.max.y - window.min.y + 1) != result.size)
                throw std::invalid_argument("Imagem EXR incompleta");
            constexpr std::array names{"R", "G", "B", "A"};
            for (const auto *name : names) {
                const auto *channel = input.header().channels().findChannel(name);
                if ((!channel && std::strcmp(name, "A") != 0) ||
                    (channel &&
                     (channel->type == Imf::UINT || channel->xSampling != 1 || channel->ySampling != 1)))
                    throw std::invalid_argument("Use um EXR RGB em ponto flutuante");
            }
            std::vector<std::array<float, 4>> row(result.size.width());
            for (int y = window.min.y; y <= window.max.y; ++y) {
                Imf::FrameBuffer frame;
                const Imath::Box2i line({window.min.x, y}, {window.max.x, y});
                for (std::size_t i = 0; i < names.size(); ++i)
                    frame.insert(names[i], Imf::Slice::Make(
                                               Imf::FLOAT, &row.front()[i], line, sizeof(row.front()),
                                               row.size() * sizeof(row.front()), 1, 1, i == 3 ? 1.0 : 0.0));
                input.setFrameBuffer(frame);
                input.readPixels(y);
                for (const auto &pixel : row)
                    for (std::size_t i = 0; i < pixel.size(); ++i) {
                        if (!std::isfinite(pixel[i]))
                            throw std::invalid_argument("Valores EXR inválidos");
                        if (i < 3)
                            result.peak = std::max(result.peak, pixel[i]);
                    }
            }
            return result;
        } catch (const std::exception &) {
            throw std::invalid_argument(
                "Imagem EXR incompleta, inválida ou sem canais RGB em ponto flutuante");
        }
    }
    initialize();
    Memory memory(FreeImage_OpenMemory(reinterpret_cast<BYTE *>(const_cast<char *>(bytes.constData())),
                                       static_cast<DWORD>(bytes.size())),
                  FreeImage_CloseMemory);
    if (!memory)
        throw std::runtime_error("Não foi possível ler a imagem de luz");
    const auto format = FreeImage_GetFileTypeFromMemory(memory.get(), 0);
    if (format != FIF_HDR)
        throw std::invalid_argument("Escolha uma imagem HDR ou EXR válida");
    FreeImage_SeekMemory(memory.get(), 0, SEEK_SET);
    HighDynamicImage result{"hdr", {}, 0};
    {
        Bitmap header(FreeImage_LoadFromMemory(format, memory.get(), FIF_LOAD_NOPIXELS), FreeImage_Unload);
        if (!header)
            throw std::invalid_argument("Cabeçalho HDR inválido");
        result.size = {int(FreeImage_GetWidth(header.get())), int(FreeImage_GetHeight(header.get()))};
    }
    if (!result.size.isValid() || result.size.width() > 8192 || result.size.height() > 8192 ||
        qint64(result.size.width()) * result.size.height() > maximumPixels)
        throw std::invalid_argument("Imagem de luz grande demais. Prefira um panorama de até 4K");
    if (decode) {
        FreeImage_SeekMemory(memory.get(), 0, SEEK_SET);
        Bitmap image(FreeImage_LoadFromMemory(format, memory.get(), 0), FreeImage_Unload);
        if (!image ||
            QSize(int(FreeImage_GetWidth(image.get())), int(FreeImage_GetHeight(image.get()))) != result.size)
            throw std::invalid_argument("Imagem HDR/EXR incompleta ou inválida");
        const auto kind = FreeImage_GetImageType(image.get());
        if (kind != FIT_RGBF && kind != FIT_RGBAF)
            throw std::invalid_argument("A imagem HDR/EXR deve conter cores RGB em ponto flutuante");
        const int channels = kind == FIT_RGBAF ? 4 : 3;
        for (int y = 0; y < result.size.height(); ++y) {
            const auto *row = reinterpret_cast<const float *>(FreeImage_GetScanLine(image.get(), y));
            for (int x = 0; x < result.size.width() * channels; ++x) {
                if (!std::isfinite(row[x]))
                    throw std::invalid_argument("Imagem HDR/EXR contém valores de luz inválidos");
                if (x % channels < 3)
                    result.peak = std::max(result.peak, row[x]);
            }
        }
    }
    return result;
}
} // namespace lmx
