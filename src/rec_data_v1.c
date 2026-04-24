static ParsedExportData_v2 ExportData_Import_v2(u8 *transfer_buf) {
    ExportData_v2 *ed = (ExportData_v2 *)transfer_buf;

    u8 *event_data_stream = calloc(ed->decompressed_event_data_stream_size + 0x100);
    u8 *compressed_data = ed->stream + ed->event_size_count*sizeof(u16) + ed->event_count*sizeof(u8);
    lz77Decompress(compressed_data, event_data_stream);

    return (ParsedExportData_v2) {
        .metadata = &ed->metadata,
        .event_size_count = ed->event_size_count,
        .event_sizes = (u16 *)ed->stream,
        .event_count = 16,
        .events = ed->stream + ed->event_size_count*sizeof(u16),
        .events = events,
        .event_data_stream = event_data_stream,
    };
}

static ParsedExportData_v2 ExportData_Import_v1(u8 *transfer_buf) {
    ExportHeader_v1 *header = (ExportHeader_v1 *)transfer_buf;
    u8 *compressed_recording = transfer_buf + header->lookup.ofst_recording;
    ExportMenuSettings_v1 *menu_settings = (ExportMenuSettings_v1 *)(transfer_buf + header->lookup.ofst_menusettings);

    // decompress
    RecordingSave_v1 *recsave = calloc(sizeof(RecordingSave_v1) + 0x100);
    lz77Decompress(compressed_recording, (u8 *)recsave);

    static const u8 events[16] = {
        RecEvent_MatchInit, RecEvent_Savestate_v1,
        RecEvent_RecordingSlot, RecEvent_RecordingSlot, RecEvent_RecordingSlot,
        RecEvent_RecordingSlot, RecEvent_RecordingSlot, RecEvent_RecordingSlot,
        RecEvent_RecordingSlot, RecEvent_RecordingSlot, RecEvent_RecordingSlot,
        RecEvent_RecordingSlot, RecEvent_RecordingSlot, RecEvent_RecordingSlot,
        RecEvent_Null, RecEvent_Null,
    };

    return (ParsedExportData_v2) {
        .metadata = &header->metadata,
        .event_size_count = countof(rec_event_data_sizes),
        .event_sizes = rec_event_data_sizes,
        .event_count = countof(events),
        .events = events,

        // We can use the recsave as an event stream directly!
        // It contains the matchinit, savestate, then 12 recording slots, all in order.
        // TODO TODO: menu settings! where do they go?
        .event_data_stream = (u8*)recsave,
    };
}

ParsedExportData_v2 ExportData_Import(u8 *transfer_buf) {
    ExportMetadata *metadata = (ExportMetadata *)transfer_buf;
    if (metadata->version == 1) return ExportData_Import_v1(transfer_buf);
    if (metadata->version == 2) return ExportData_Import_v2(transfer_buf);
    return (ParsedExportData_v2) { 0 };
}

// works for both versions (FOR NOW!)
void ExportData_Release(ParsedExportData_v2 *export_data) {
    HSD_Free(export_data->event_data_stream); // free decompressed buffer
}
