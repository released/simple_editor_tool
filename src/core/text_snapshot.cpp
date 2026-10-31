#include "text_snapshot.h"
namespace simple_editor::core {
bool TextInputSnapshot::Materialize(const std::atomic_bool& cancel,std::wstring* error) {
    if (!document || cancel) return false;
    if (dirty) {
        std::vector<std::uint8_t> bytes(displayed.begin(),displayed.end()); std::wstring text; TextCodec utf8;
        if (!DecodeText(bytes,TextEncoding::Utf8,&text,&utf8,error) || cancel ||
            !EncodeText(TextFromEditControl(original,text),codec,true,&bytes,error) || cancel ||
            !document->Replace(0,document->Size(),bytes)) {
            if (error && error->empty()) *error = L"Could not prepare the text snapshot. The editor draft is retained.";
            return false;
        }
        dirty = false;
    }
    return !cancel && document->ValidateSource(true,error);
}
bool PreparedTextSideCopy::Prepare(const std::atomic_bool& cancel,std::wstring* error) {
    prepared = false;
    const auto fail = [&](const wchar_t* message) { if (error) *error = message; return false; };
    for (unsigned side = 0; side < 2; ++side) {
        auto input = inputs[side]; std::vector<std::uint8_t> bytes; std::wstring raw; TextCodec codec;
        if (!input || !input->Materialize(cancel,error) || input->document->Size() > 16 * 1024 * 1024 ||
            !input->document->Read(0,static_cast<std::size_t>(input->document->Size()),&bytes) ||
            !DecodeText(bytes,(side ? right_codec : left_codec).encoding,&raw,&codec,error) ||
            !input->document->ValidateSource(true,error)) return fail(L"Could not validate the comparison sources. No content was copied.");
        if (raw != (side ? right : left)) return fail(L"A source or target changed since comparison. Compare again before copying.");
        if (static_cast<bool>(side) == to_right) {
            if (input->session && !input->loaded) {
                std::wstring native;
                if (!DecodeText(bytes,input->requested,&native,&target_codec,error) || native != raw)
                    return fail(L"The target editor uses different text decoding. Select matching encodings and compare again.");
            } else {
                target_codec = input->session ? input->codec : codec;
                if (input->session) {
                    std::vector<std::uint8_t> native(input->displayed.begin(),input->displayed.end()); std::wstring native_text; TextCodec utf8;
                    if (!DecodeText(native,TextEncoding::Utf8,&native_text,&utf8,error) || TextFromEditControl(input->original,native_text) != raw)
                        return fail(L"The target editor uses different text decoding. Select matching encodings and compare again.");
                }
            }
            initial = raw;
        }
    }
    displayed = TextForEditControl(replacement); std::vector<std::uint8_t> encoded;
    const auto target = inputs[to_right ? 1 : 0];
    const auto& original = target->session && target->loaded ? target->original : initial;
    if (cancel || !EncodeText(TextFromEditControl(original,displayed),target_codec,true,&encoded,error)) return false;
    if (encoded.size() > 16 * 1024 * 1024) return fail(L"The copied text exceeds the 16 MiB comparison input limit. No content was copied.");
    prepared = !cancel; return prepared;
}
}
