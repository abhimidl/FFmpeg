/*
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_MUX_H
#define AVFORMAT_MUX_H

#include <stdint.h>
#include "libavcodec/packet.h"
#include "avformat.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

size_t strftime_millis(char *ptr, size_t maxsize, const char *format, const struct timeval *tv);

struct AVDeviceInfoList;

#define FF_FMT_ALLOW_FLUSH (1 << 1)

typedef struct FFOutputFormat
{
    /**
     * The public AVOutputFormat. See avformat.h for it.
     */
    AVOutputFormat p;
    /**
     * size of private data so that it can be allocated in the wrapper
     */
    int priv_data_size;

    /**
     * Internal flags. See FF_FMT_* in internal.h and mux.h.
     */
    int flags_internal;

    int (*write_header)(AVFormatContext *);
    /**
     * Write a packet. If FF_FMT_ALLOW_FLUSH is set in flags_internal,
     * pkt can be NULL in order to flush data buffered in the muxer.
     * When flushing, return 0 if there still is more data to flush,
     * or 1 if everything was flushed and there is no more buffered
     * data.
     */
    int (*write_packet)(AVFormatContext *, AVPacket *pkt);
    int (*write_trailer)(AVFormatContext *);
    /**
     * A format-specific function for interleavement.
     * If unset, packets will be interleaved by dts.
     *
     * @param s           An AVFormatContext for output. pkt will be added to
     *                    resp. taken from its packet buffer.
     * @param[in,out] pkt A packet to be interleaved if has_packet is set;
     *                    also used to return packets. If no packet is returned
     *                    (e.g. on error), pkt is blank on return.
     * @param flush       1 if no further packets are available as input and
     *                    all remaining packets should be output.
     * @param has_packet  If set, pkt contains a packet to be interleaved
     *                    on input; otherwise pkt is blank on input.
     * @return 1 if a packet was output, 0 if no packet could be output,
     *         < 0 if an error occurred
     */
    int (*interleave_packet)(AVFormatContext *s, AVPacket *pkt,
                             int flush, int has_packet);
    /**
     * Test if the given codec can be stored in this container.
     *
     * @return 1 if the codec is supported, 0 if it is not.
     *         A negative number if unknown.
     *         MKTAG('A', 'P', 'I', 'C') if the codec is only supported as AV_DISPOSITION_ATTACHED_PIC
     */
    int (*query_codec)(enum AVCodecID id, int std_compliance);

    void (*get_output_timestamp)(AVFormatContext *s, int stream,
                                 int64_t *dts, int64_t *wall);
    /**
     * Allows sending messages from application to device.
     */
    int (*control_message)(AVFormatContext *s, int type,
                           void *data, size_t data_size);

    /**
     * Write an uncoded AVFrame.
     *
     * See av_write_uncoded_frame() for details.
     *
     * The library will free *frame afterwards, but the muxer can prevent it
     * by setting the pointer to NULL.
     */
    int (*write_uncoded_frame)(AVFormatContext *, int stream_index,
                               struct AVFrame **frame, unsigned flags);
    /**
     * Returns device list with it properties.
     * @see avdevice_list_devices() for more details.
     */
    int (*get_device_list)(AVFormatContext *s, struct AVDeviceInfoList *device_list);
    /**
     * Initialize format. May allocate data here, and set any AVFormatContext or
     * AVStream parameters that need to be set before packets are sent.
     * This method must not write output.
     *
     * Return 0 if streams were fully configured, 1 if not, negative AVERROR on failure
     *
     * Any allocations made here must be freed in deinit().
     */
    int (*init)(AVFormatContext *);
    /**
     * Deinitialize format. If present, this is called whenever the muxer is being
     * destroyed, regardless of whether or not the header has been written.
     *
     * If a trailer is being written, this is called after write_trailer().
     *
     * This is called if init() fails as well.
     */
    void (*deinit)(AVFormatContext *);
    /**
     * Set up any necessary bitstream filtering and extract any extra data needed
     * for the global header.
     *
     * @note pkt might have been directly forwarded by a meta-muxer; therefore
     *       pkt->stream_index as well as the pkt's timebase might be invalid.
     * Return 0 if more packets from this stream must be checked; 1 if not.
     */
    int (*check_bitstream)(AVFormatContext *s, AVStream *st,
                           const AVPacket *pkt);
} FFOutputFormat;

static inline const FFOutputFormat *ffofmt(const AVOutputFormat *fmt)
{
    return (const FFOutputFormat *)fmt;
}

/**
 * Add packet to an AVFormatContext's packet_buffer list, determining its
 * interleaved position using compare() function argument.
 * @return 0 on success, < 0 on error. pkt will always be blank on return.
 */
int ff_interleave_add_packet(AVFormatContext *s, AVPacket *pkt,
                             int (*compare)(AVFormatContext *, const AVPacket *, const AVPacket *));

/**
 * Interleave an AVPacket per dts so it can be muxed.
 * See the documentation of AVOutputFormat.interleave_packet for details.
 */
int ff_interleave_packet_per_dts(AVFormatContext *s, AVPacket *pkt,
                                 int flush, int has_packet);

/**
 * Interleave packets directly in the order in which they arrive
 * without any sort of buffering.
 */
int ff_interleave_packet_passthrough(AVFormatContext *s, AVPacket *pkt,
                                     int flush, int has_packet);

/**
 * Find the next packet in the interleaving queue for the given stream.
 *
 * @return a pointer to a packet if one was found, NULL otherwise.
 */
const AVPacket *ff_interleaved_peek(AVFormatContext *s, int stream);

int ff_get_muxer_ts_offset(AVFormatContext *s, int stream_index, int64_t *offset);

/**
 * Add a bitstream filter to a stream.
 *
 * @param st output stream to add a filter to
 * @param name the name of the filter to add
 * @param args filter-specific argument string
 * @return  >0 on success;
 *          AVERROR code on failure
 */
int ff_stream_add_bitstream_filter(AVStream *st, const char *name, const char *args);

/**
 * Write a packet to another muxer than the one the user originally
 * intended. Useful when chaining muxers, where one muxer internally
 * writes a received packet to another muxer.
 *
 * @param dst the muxer to write the packet to
 * @param dst_stream the stream index within dst to write the packet to
 * @param pkt the packet to be written. It will be returned blank when
 *            av_interleaved_write_frame() is used, unchanged otherwise.
 * @param src the muxer the packet originally was intended for
 * @param interleave 0->use av_write_frame, 1->av_interleaved_write_frame
 * @return the value av_write_frame returned
 */
int ff_write_chained(AVFormatContext *dst, int dst_stream, AVPacket *pkt,
                     AVFormatContext *src, int interleave);

/**
 * Flags for AVFormatContext.write_uncoded_frame()
 */
enum AVWriteUncodedFrameFlags
{

    /**
     * Query whether the feature is possible on this stream.
     * The frame argument is ignored.
     */
    AV_WRITE_UNCODED_FRAME_QUERY = 0x0001,

};

/**
 * Make shift_size amount of space at read_start by shifting data in the output
 * at read_start until the current IO position. The underlying IO context must
 * be seekable.
 */
int ff_format_shift_data(AVFormatContext *s, int64_t read_start, int shift_size);

/**
 * Utility function to open IO stream of output format.
 *
 * @param s AVFormatContext
 * @param url URL or file name to open for writing
 * @options optional options which will be passed to io_open callback
 * @return >=0 on success, negative AVERROR in case of failure
 */
int ff_format_output_open(AVFormatContext *s, const char *url, AVDictionary **options);

/**
 * Parse creation_time in AVFormatContext metadata if exists and warn if the
 * parsing fails.
 *
 * @param s AVFormatContext
 * @param timestamp parsed timestamp in microseconds, only set on successful parsing
 * @param return_seconds set this to get the number of seconds in timestamp instead of microseconds
 * @return 1 if OK, 0 if the metadata was not present, AVERROR(EINVAL) on parse error
 */
int ff_parse_creation_time_metadata(AVFormatContext *s, int64_t *timestamp, int return_seconds);

/**
 * Standardize creation_time metadata in AVFormatContext to an ISO-8601
 * timestamp string.
 *
 * @param s AVFormatContext
 * @return <0 on error
 */
int ff_standardize_creation_time(AVFormatContext *s);

#endif /* AVFORMAT_MUX_H */
