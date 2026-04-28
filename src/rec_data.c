#include "../MexTK/mex.h"
#include "events.h"
#include "rec_data.h"

static u32 lz77Compress(u8 *uncompressed_text, u32 uncompressed_size, u8 *compressed_text, u8 pointer_length_width);
static u32 lz77Decompress(u8 *compressed_text, u8 *uncompressed_text);

u32 rec_event_data_sizes[RecEvent_Count] = {
    0,
    sizeof(RecEventData_MatchInit),
    sizeof(RecEventData_Savestate_v1),
    sizeof(RecEventData_Savestate_v2),
    sizeof(RecEventData_RecordingSlot_v1),
    sizeof(RecEventData_MenuSettings_Record_v1),
};

static ParsedExportData_v2 ExportData_Import_v2(u8 *transfer_buf) {
    ExportData_v2 *ed = (ExportData_v2 *)transfer_buf;

    u8 *event_data_stream = calloc(ed->decompressed_event_data_stream_size + 0x100);
    u8 *compressed_data = ed->stream + ed->event_size_count*sizeof(u32) + ed->event_count*sizeof(u8);
    lz77Decompress(compressed_data, event_data_stream);

    return (ParsedExportData_v2) {
        .metadata = &ed->metadata,
        .event_size_count = ed->event_size_count,
        .event_sizes = (u32 *)ed->stream,
        .event_count = 16,
        .events = ed->stream + ed->event_size_count*sizeof(u32),
        .event_data_stream = event_data_stream,
    };
}

static ParsedExportData_v2 ExportData_Import_v1(u8 *transfer_buf) {
    ExportHeader_v1 *header = (ExportHeader_v1 *)transfer_buf;
    u8 *compressed_recording = transfer_buf + header->lookup.ofst_recording;
    ExportMenuSettings_v1 *menu_settings = (ExportMenuSettings_v1 *)(transfer_buf + header->lookup.ofst_menusettings);

    // decompress
    RecordingSave_v1 *recsave = calloc(sizeof(RecordingSave_v1) + 0x100);
    u32 decompressed_size = (u32)lz77Decompress(compressed_recording, (u8 *)recsave);

    static u8 events[16] = {
        RecEvent_MatchInit, RecEvent_Savestate_v1,
        RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1,
        RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1,
        RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1,
        RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1, RecEvent_RecordingSlot_v1,
        RecEvent_MenuSettings_Record_v1, RecEvent_Null,
    };

    // append menu settings
    RecEventData_MenuSettings_Record_v1* record = (void*)&recsave[decompressed_size];
    record->menu_settings = *menu_settings;

    return (ParsedExportData_v2) {
        .metadata = &header->metadata,
        .event_size_count = countof(rec_event_data_sizes),
        .event_sizes = rec_event_data_sizes,
        .event_count = countof(events),
        .events = events,

        // We can use the recsave as an event stream directly!
        // It contains the matchinit, savestate, 12 recording slots, then menu settings, all in order.
        .event_data_stream = (u8*)recsave,
    };
}

ParsedExportData_v2 ExportData_Import(u8 *transfer_buf) {
    ExportMetadata *metadata = (ExportMetadata *)transfer_buf;
    if (metadata->version == 1) return ExportData_Import_v1(transfer_buf);
    if (metadata->version == 2) return ExportData_Import_v2(transfer_buf);
    // TODO old/new checking
    return (ParsedExportData_v2) { 0 };
}

// works for both versions (FOR NOW!)
void ExportData_Free(ParsedExportData_v2 *export_data) {
    HSD_Free(export_data->event_data_stream); // free decompressed buffer
}

int ExportData_Compress(u8 *dst, u8 *src, u32 size) {
    return lz77Compress(src, size, dst, 8); // TODO what is pointer_length_width?
}

static int pow_n(int x, int n) {
    int res = 1;
    for (; n; n--)
        res *= x;
    return res;
}

static u32 lz77Compress(u8 *uncompressed_text, u32 uncompressed_size, u8 *compressed_text, u8 pointer_length_width) {
    u16 pointer_pos, temp_pointer_pos, output_pointer, pointer_length, temp_pointer_length;
    u32 compressed_pointer, output_size, coding_pos, output_lookahead_ref, look_behind, look_ahead;
    u16 pointer_pos_max, pointer_length_max;
    pointer_pos_max = pow_n(2, 16 - pointer_length_width);
    pointer_length_max = pow_n(2, pointer_length_width);

    *((u32 *)compressed_text) = uncompressed_size;
    *(compressed_text + 4) = pointer_length_width;
    compressed_pointer = output_size = 5;

    for (coding_pos = 0; coding_pos < uncompressed_size; ++coding_pos)
    {
        pointer_pos = 0;
        pointer_length = 0;
        for (temp_pointer_pos = 1; (temp_pointer_pos < pointer_pos_max) && (temp_pointer_pos <= coding_pos); ++temp_pointer_pos)
        {
            look_behind = coding_pos - temp_pointer_pos;
            look_ahead = coding_pos;
            for (temp_pointer_length = 0; uncompressed_text[look_ahead++] == uncompressed_text[look_behind++]; ++temp_pointer_length)
                if (temp_pointer_length == pointer_length_max)
                    break;
            if (temp_pointer_length > pointer_length)
            {
                pointer_pos = temp_pointer_pos;
                pointer_length = temp_pointer_length;
                if (pointer_length == pointer_length_max)
                    break;
            }
        }
        coding_pos += pointer_length;
        if ((coding_pos == uncompressed_size) && pointer_length)
        {
            output_pointer = (pointer_length == 1) ? 0 : ((pointer_pos << pointer_length_width) | (pointer_length - 2));
            output_lookahead_ref = coding_pos - 1;
        }
        else
        {
            output_pointer = (pointer_pos << pointer_length_width) | (pointer_length ? (pointer_length - 1) : 0);
            output_lookahead_ref = coding_pos;
        }
        *((u16 *)(compressed_text + compressed_pointer)) = output_pointer;
        compressed_pointer += 2;
        *(compressed_text + compressed_pointer++) = *(uncompressed_text + output_lookahead_ref);
        output_size += 3;
    }

    return output_size;
}

// lz77 functions credited to https://github.com/andyherbert/lz1
static u32 lz77Decompress(u8 *compressed_text, u8 *uncompressed_text) {
    u8 pointer_length_width;
    u16 input_pointer, pointer_length, pointer_pos, pointer_length_mask;
    u32 compressed_pointer, coding_pos, pointer_offset, uncompressed_size;

    uncompressed_size = *((u32 *)compressed_text);
    pointer_length_width = *(compressed_text + 4);
    compressed_pointer = 5;

    pointer_length_mask = pow_n(2, pointer_length_width) - 1;

    for (coding_pos = 0; coding_pos < uncompressed_size; ++coding_pos)
    {
        input_pointer = *((u16 *)(compressed_text + compressed_pointer));
        compressed_pointer += 2;
        pointer_pos = input_pointer >> pointer_length_width;
        pointer_length = pointer_pos ? ((input_pointer & pointer_length_mask) + 1) : 0;
        if (pointer_pos)
            for (pointer_offset = coding_pos - pointer_pos; pointer_length > 0; --pointer_length)
                uncompressed_text[coding_pos++] = uncompressed_text[pointer_offset++];
        *(uncompressed_text + coding_pos) = *(compressed_text + compressed_pointer++);
    }

    return coding_pos;
}

