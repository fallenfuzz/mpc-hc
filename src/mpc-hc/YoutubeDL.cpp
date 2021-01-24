/*
* (C) 2018 Nicholas Parkanyi
*
* This file is part of MPC-HC.
*
* MPC-HC is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* MPC-HC is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "stdafx.h"
#include "YoutubeDL.h"
#include "rapidjson/include/rapidjson/document.h"
#include "mplayerc.h"
#include "logger.h"

typedef rapidjson::GenericValue<rapidjson::UTF16<>> Value;

struct CUtf16JSON {
    rapidjson::GenericDocument<rapidjson::UTF16<>> d;
};


CYoutubeDLInstance::CYoutubeDLInstance()
    : idx_out(0), idx_err(0),
      buf_out(nullptr), buf_err(nullptr),
      capacity_out(0), capacity_err(0),
      pJSON(new CUtf16JSON)
{
}

CYoutubeDLInstance::~CYoutubeDLInstance()
{
    std::free(buf_out);
    std::free(buf_err);
    delete pJSON;
}

bool CYoutubeDLInstance::Run(CString url)
{
    const size_t bufsize = 2000;  //2KB initial buffer size

    /////////////////////////////
    // Set up youtube-dl process
    /////////////////////////////

    PROCESS_INFORMATION proc_info;
    STARTUPINFO startup_info;
    SECURITY_ATTRIBUTES sec_attrib;

    YDL_LOG(url);

    CString args = _T("youtube-dl -J --all-subs --no-warnings --youtube-skip-dash-manifest");
    if (url.Find(_T("list=")) > 0) {
        args.Append(_T(" --ignore-errors"));
    }
    args.Append(_T(" \"") + url + _T("\""));

    ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&startup_info, sizeof(STARTUPINFO));

    //child process must inherit the handles
    sec_attrib.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec_attrib.lpSecurityDescriptor = NULL;
    sec_attrib.bInheritHandle = true;

    if (!CreatePipe(&hStdout_r, &hStdout_w, &sec_attrib, bufsize)) {
        return false;
    }
    if (!CreatePipe(&hStderr_r, &hStderr_w, &sec_attrib, bufsize)) {
        return false;
    }

    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdOutput = hStdout_w;
    startup_info.hStdError = hStderr_w;
    startup_info.wShowWindow = SW_HIDE;
    startup_info.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

    if (!CreateProcess(NULL, args.GetBuffer(), NULL, NULL, true, 0,
                       NULL, NULL, &startup_info, &proc_info)) {
        YDL_LOG(_T("Failed to run YDL"));
        return false;
    }

    //we must close the parent process's write handles before calling ReadFile,
    // otherwise it will block forever.
    CloseHandle(hStdout_w);
    CloseHandle(hStderr_w);


    /////////////////////////////////////////////////////
    // Read in stdout and stderr through the pipe buffer
    /////////////////////////////////////////////////////

    buf_out = static_cast<char*>(std::malloc(bufsize));
    buf_err = static_cast<char*>(std::malloc(bufsize));
    capacity_out = bufsize;
    capacity_err = bufsize;

    HANDLE hThreadOut, hThreadErr;
    idx_out = 0;
    idx_err = 0;

    hThreadOut = CreateThread(NULL, 0, BuffOutThread, this, NULL, NULL);
    hThreadErr = CreateThread(NULL, 0, BuffErrThread, this, NULL, NULL);

    WaitForSingleObject(hThreadOut, INFINITE);
    WaitForSingleObject(hThreadErr, INFINITE);

    if (!buf_out || !buf_err) {
        throw std::bad_alloc();
    }

    //NULL-terminate the data
    char* tmp;
    if (idx_out == capacity_out) {
        tmp = static_cast<char*>(std::realloc(buf_out, capacity_out + 1));
        if (tmp) {
            buf_out = tmp;
        }
    }
    buf_out[idx_out] = '\0';

    if (idx_err == capacity_err) {
        tmp = static_cast<char*>(std::realloc(buf_err, capacity_err + 1));
        if (tmp) {
            buf_err = tmp;
        }
    }
    buf_err[idx_err] = '\0';

    DWORD exitcode;
    GetExitCodeProcess(proc_info.hProcess, &exitcode);

    CloseHandle(proc_info.hProcess);
    CloseHandle(proc_info.hThread);
    CloseHandle(hThreadOut);
    CloseHandle(hThreadErr);
    CloseHandle(hStdout_r);
    CloseHandle(hStderr_r);

    if (exitcode) {
        CString err = buf_err;
        if (err.IsEmpty()) {
            err.Format(_T("An error occurred while running youtube-dl.exe\n\nprocess exitcode = %d"), exitcode);
        } else {
            if (err.Find(_T("ERROR: Unsupported URL")) >= 0) {
                // abort without showing error message
                return false;
            }
            err = _T("Youtube-dl.exe error message:\n\n") + err;
        }
        AfxMessageBox(err, MB_ICONERROR, 0);
        return false;
    }

    return loadJSON();
}

DWORD WINAPI CYoutubeDLInstance::BuffOutThread(void* ydl_inst)
{
    auto ydl = static_cast<CYoutubeDLInstance*>(ydl_inst);
    DWORD read;

    while (ReadFile(ydl->hStdout_r, ydl->buf_out + ydl->idx_out, ydl->capacity_out - ydl->idx_out, &read, NULL)) {
        ydl->idx_out += read;
        if (ydl->idx_out == ydl->capacity_out) {
            ydl->capacity_out *= 2;
            char* tmp = static_cast<char*>(std::realloc(ydl->buf_out, ydl->capacity_out));
            if (tmp) {
                ydl->buf_out = tmp;
            } else {
                std::free(ydl->buf_out);
                ydl->buf_out = nullptr;
                return 0;
            }
        }
    }

    return GetLastError() == ERROR_BROKEN_PIPE ? 0 : GetLastError();
}

DWORD WINAPI CYoutubeDLInstance::BuffErrThread(void* ydl_inst)
{
    auto ydl = static_cast<CYoutubeDLInstance*>(ydl_inst);
    DWORD read;

    while (ReadFile(ydl->hStderr_r, ydl->buf_err + ydl->idx_err, ydl->capacity_err - ydl->idx_err, &read, NULL)) {
        ydl->idx_err += read;
        if (ydl->idx_err == ydl->capacity_err) {
            ydl->capacity_err *= 2;
            char* tmp = static_cast<char*>(std::realloc(ydl->buf_err, ydl->capacity_err));
            if (tmp) {
                ydl->buf_err = tmp;
            } else {
                std::free(ydl->buf_err);
                ydl->buf_err = nullptr;
                return 0;
            }
        }
    }

    return GetLastError() == ERROR_BROKEN_PIPE ? 0 : GetLastError();
}

struct YDLStreamDetails {
    CString protocol;
    CString url;
    int width;
    int height;
    CString vcodec;
    CString acodec;
    CString format;
    bool has_video;
    bool has_audio;
    int vbr;
    int abr;
    int fps;
};

#define YDL_EXTRA_LOGGING 0
#define YDL_LOG_URLS      1

bool GetYDLStreamDetails(const Value& format, YDLStreamDetails& details, bool require_video, bool require_audio_only)
{
    details.protocol = format.HasMember(_T("protocol")) && !format[_T("protocol")].IsNull() ? format[_T("protocol")].GetString() : nullptr;
    if (details.protocol && details.protocol != _T("http_dash_segments")) {
        details.url       = format[_T("url")].GetString();
        if (details.url.IsEmpty()) return false;

        details.width     = format.HasMember(_T("width"))  && !format[_T("width")].IsNull()  ? format[_T("width")].GetInt() : 0;
        details.height    = format.HasMember(_T("height")) && !format[_T("height")].IsNull() ? format[_T("height")].GetInt() : 0;
        details.vcodec    = format.HasMember(_T("vcodec")) && !format[_T("vcodec")].IsNull() ? format[_T("vcodec")].GetString() : _T("none");
        details.has_video = details.vcodec != _T("none") || (details.width > 0) || (details.height > 0);
        details.acodec    = format.HasMember(_T("acodec")) && !format[_T("acodec")].IsNull() ? format[_T("acodec")].GetString() : _T("none");
        details.has_audio = details.acodec != _T("none");
        details.format    = format.HasMember(_T("format")) && !format[_T("format")].IsNull() ? format[_T("format")].GetString() : _T("none");

        if (require_video && !details.has_video) {
            #if YDL_EXTRA_LOGGING
            YDL_LOG(_T("ignore url because it has no video: %s"), static_cast<LPCWSTR>(details.url));
            #endif
            return false;
        }
        if (require_audio_only && (details.has_video || !details.has_audio)) {
            #if YDL_EXTRA_LOGGING
            YDL_LOG(_T("ignore url because it is not audio only (vcodec=%s): %s"), static_cast<LPCWSTR>(details.vcodec), static_cast<LPCWSTR>(details.url));
            #endif
            return false;
        }

        details.vbr = details.has_video && format.HasMember(_T("vbr")) && !format[_T("vbr")].IsNull() ? (int)format[_T("vbr")].GetFloat() : 0;
        if (details.vbr == 0 && details.has_video) {
            details.vbr = format.HasMember(_T("tbr")) && !format[_T("tbr")].IsNull() ? (int)format[_T("tbr")].GetFloat() : 0;
        }
        details.fps = format.HasMember(_T("fps")) && !format[_T("fps")].IsNull() ? (int)format[_T("fps")].GetDouble() : 0;
        details.abr = details.has_audio && format.HasMember(_T("abr")) && !format[_T("abr")].IsNull() ? (int)format[_T("abr")].GetFloat() : 0;
        if (details.abr == 0 && details.has_audio) {
            details.abr = format.HasMember(_T("tbr")) && !format[_T("tbr")].IsNull() ? (int)format[_T("tbr")].GetFloat() : 0;
        }

        #if YDL_LOG_URLS
        YDL_LOG(_T("vcodec=%s width=%d height=%d fps=%d vbr=%d acodec=%s abr=%d url=%s"), static_cast<LPCWSTR>(details.vcodec), details.width, details.height, details.fps, details.vbr, static_cast<LPCWSTR>(details.acodec), details.abr, static_cast<LPCWSTR>(details.url));
        #else
        YDL_LOG(_T("vcodec=%s width=%d height=%d fps=%d vbr=%d acodec=%s abr=%d"), static_cast<LPCWSTR>(details.vcodec), details.width, details.height, details.fps, details.vbr, static_cast<LPCWSTR>(details.acodec), details.abr);
        #endif

        return true;
    }
    else {
        #if 0
        YDL_LOG(_T("ignore dash url = %s"), format[_T("url")].GetString());
        #endif
    }
    return false;
}

#define YDL_FORMAT_AUTO      0
#define YDL_FORMAT_H264_30   1
#define YDL_FORMAT_H264_60   2
#define YDL_FORMAT_VP9_30    3
#define YDL_FORMAT_VP9_60    4
#define YDL_FORMAT_VP9P2_30  5
#define YDL_FORMAT_VP9P2_60  6
#define YDL_FORMAT_AV1_30    7
#define YDL_FORMAT_AV1_60    8

bool IsBetterYDLStream(YDLStreamDetails& first, YDLStreamDetails& second, int max_height, bool separate, int preferred_format)
{
    if (first.has_video) {
        // We want separate audio/video streams
        if (separate && first.has_audio && !second.has_audio) {
            return true;
        }

        if (!second.has_video) {
            return false;
        }

        // Video format
        CString vcodec1 = first.vcodec.Left(4);
        CString vcodec2 = second.vcodec.Left(4);
        if (vcodec1 != vcodec2) {
            // AV1
            if (vcodec1 == _T("av01")) {
                return (preferred_format != YDL_FORMAT_AV1_30 && preferred_format != YDL_FORMAT_AV1_60);
            } else {
                if (vcodec2 == _T("av01")) {
                    return (preferred_format == YDL_FORMAT_AV1_30 || preferred_format == YDL_FORMAT_AV1_60);
                }
            }
            // H.264
            if ((preferred_format == YDL_FORMAT_H264_30 || preferred_format == YDL_FORMAT_H264_60)) {
                if (vcodec1 == _T("avc1")) {
                    return false;
                } else {
                    if (vcodec2 == _T("avc1")) {
                        return true;
                    }
                }
            }
        }
        if (first.vcodec != second.vcodec) {
            // VP9P2
            if ((preferred_format == YDL_FORMAT_VP9P2_30 || preferred_format == YDL_FORMAT_VP9P2_60)) {
                if (first.vcodec == _T("vp9.2")) {
                    return false;
                } else {
                    if (second.vcodec == _T("vp9.2")) {
                        return true;
                    }
                }
                // Prefer VP9P0 over others
                if (first.vcodec.Left(3) == _T("vp9")) {
                    return false;
                } else {
                    if (second.vcodec.Left(3) == _T("vp9")) {
                        return true;
                    }
                }
            }
            // VP9
            if ((preferred_format == YDL_FORMAT_VP9_30 || preferred_format == YDL_FORMAT_VP9_60)) {
                if (first.vcodec == _T("vp9") || first.vcodec == _T("vp9.0")) {
                    return false;
                } else {
                    if (second.vcodec == _T("vp9") || second.vcodec == _T("vp9.0")) {
                        return true;
                    }
                }
            }
        }

        // Video resolution
        if (max_height > 0 && first.height > max_height && first.height > second.height) {
            return true;
        }
        if (max_height > 0 && second.height > max_height) {
            return false;
        }
        if (second.height > first.height) {
            if (max_height > 0) {
                // calculate maximum width based on 16:9 AR
                return (max_height * 16 / 9 + 1) >= second.width;
            }
            return true;
        } else {
            if (second.height == first.height) {
                if (second.width > first.width) {
                    return true;
                }
                if (second.width < first.width) {
                    return false;
                }
            } else {
                return false;
            }
        }

        // Framerate
        if (preferred_format != YDL_FORMAT_AUTO && first.fps != second.fps && first.fps > 0 && second.fps > 0) {
            if (preferred_format == YDL_FORMAT_H264_60 || preferred_format == YDL_FORMAT_VP9_60 || preferred_format == YDL_FORMAT_VP9P2_60 || preferred_format == YDL_FORMAT_AV1_60) {
                if (second.fps > first.fps) {
                    return true;
                } else if (first.fps > second.fps) {
                    return false;
                }
            } else if (preferred_format == YDL_FORMAT_H264_30 || preferred_format == YDL_FORMAT_VP9_30 || preferred_format == YDL_FORMAT_VP9P2_30 || preferred_format == YDL_FORMAT_AV1_30) {
                if (first.fps > 30 && first.fps > second.fps) {
                    return true;
                } else if (second.fps > 30 && second.fps > first.fps) {
                    return false;
                }
            }
        }
    } else if (first.has_audio) {
        if (!second.has_audio) {
            return false;
        }

        // Audio format
        if (first.acodec.Left(4) == _T("opus")) {
            if (second.acodec.Left(4) != _T("opus")) {
                return false;
            }
        } else {
            if (second.acodec.Left(4) == _T("opus")) {
                return true;
            }
        }
    } else {
        if (second.has_video || second.has_audio) {
            return true;
        } else {
            if (first.format != _T("none")) {
                return false;
            } else {
                if (second.format != _T("none")) return true;
            }
        }
    }

    // Prefer HTTP protocol
    if (first.protocol.Left(4) == _T("http")) {
        if (second.protocol.Left(4) != _T("http")) {
            return false;
        }
    } else {
        if (second.protocol.Left(4) == _T("http")) {
            return true;
        }
    }

    // Bitrate
    if (first.has_video) {
        if (second.vbr > first.vbr) {
            return true;
        }
    } else {
        if (second.abr > first.abr) {
            return true;
        }
    }
    return false;
}

// find best video track
bool filterVideo(const Value& entry, YDLStreamDetails& ydl_sd, int max_height, bool separate, int preferred_format)
{
    YDLStreamDetails current;
    bool found = false;
    if (entry.HasMember(_T("formats")) && !entry[_T("formats")].IsNull() && entry[_T("formats")].IsArray()) {
        const Value& formats = entry[_T("formats")];
        for (rapidjson::SizeType i = 0; i < formats.Size(); i++) {
            if (GetYDLStreamDetails(formats[i], current, true, false)) {
                if (!found || IsBetterYDLStream(ydl_sd, current, max_height, separate, preferred_format)) {
                    ydl_sd = current;
                    //YDL_LOG(_T("this is currently best video stream"));
                }
                found = true;
            }
        }
    } else if (entry.HasMember(_T("url")) && !entry[_T("url")].IsNull()) {
        current.url = entry[_T("url")].GetString();
        ydl_sd = current;
        found = true;
    }
    
    return found;
}

// find best audio track (in case we use separate streams)
bool filterAudio(const Value& formats, YDLStreamDetails& ydl_sd)
{
    YDLStreamDetails current;
    bool found = false;

    for (rapidjson::SizeType i = 0; i < formats.Size(); i++) {
        if (GetYDLStreamDetails(formats[i], current, false, true)) {
            if (!found || IsBetterYDLStream(ydl_sd, current, 0, true, 0)) {
                ydl_sd = current;
                //YDL_LOG(_T("this is currently best audio stream"));
            }
            found = true;
        }
    }
    return found;
}

bool CYoutubeDLInstance::GetHttpStreams(CAtlList<YDLStreamURL>& streams, YDLPlaylistInfo& info)
{
    CString url;
    CString extractor;
    YDLStreamDetails ydl_sd;
    YDLStreamURL stream;

    if (pJSON->d.IsObject() && pJSON->d.HasMember(_T("extractor"))) {
        extractor = pJSON->d[_T("extractor")].GetString();
    } else {
        return false;
    }

    auto& s = AfxGetAppSettings();

    if (!bIsPlaylist) {
        if ((!pJSON->d.HasMember(_T("formats")) || pJSON->d[_T("formats")].IsNull())&& (!pJSON->d.HasMember(_T("url")) || pJSON->d[_T("url")].IsNull())) {
            return false;
        }

        if (pJSON->d.HasMember(_T("title")) && !pJSON->d[_T("title")].IsNull()) {
            stream.title = pJSON->d[_T("title")].GetString();
        } else {
            stream.title = _T("");
        }

        if (pJSON->d.HasMember(_T("series")) && !pJSON->d[_T("series")].IsNull()) stream.series = pJSON->d[_T("series")].GetString();
        if (pJSON->d.HasMember(_T("season")) && !pJSON->d[_T("season")].IsNull()) stream.season = pJSON->d[_T("season")].GetString();
        if (pJSON->d.HasMember(_T("season_number")) && !pJSON->d[_T("season_number")].IsNull()) stream.season_number = pJSON->d[_T("season_number")].GetInt();
        if (pJSON->d.HasMember(_T("season_id")) && !pJSON->d[_T("season_id")].IsNull()) stream.season_id = pJSON->d[_T("season_id")].GetString();
        if (pJSON->d.HasMember(_T("episode")) && !pJSON->d[_T("episode")].IsNull()) stream.episode = pJSON->d[_T("episode")].GetString();
        if (pJSON->d.HasMember(_T("episode_number")) && !pJSON->d[_T("episode_number")].IsNull()) stream.episode_number = pJSON->d[_T("episode_number")].GetInt();
        if (pJSON->d.HasMember(_T("episode_id")) && !pJSON->d[_T("episode_id")].IsNull()) stream.episode_id = pJSON->d[_T("episode_id")].GetString();
        if (pJSON->d.HasMember(_T("webpage_url")) && !pJSON->d[_T("webpage_url")].IsNull()) stream.webpage_url = pJSON->d[_T("webpage_url")].GetString();

        if (filterVideo(pJSON->d, ydl_sd, s.iYDLMaxHeight, s.bYDLAudioOnly, s.iYDLVideoFormat)) {
            stream.video_url = ydl_sd.url;
            stream.audio_url = _T("");
            // find separate audio stream
            if (ydl_sd.has_video && !ydl_sd.has_audio && pJSON->d.HasMember(_T("formats")) && !pJSON->d[_T("formats")].IsNull()) {
                if (filterAudio(pJSON->d[_T("formats")], ydl_sd)) {
                    stream.audio_url = ydl_sd.url;
                }
            }
            streams.AddTail(stream);
        } else if (pJSON->d.HasMember(_T("formats")) && !pJSON->d[_T("formats")].IsNull()) {
            if (filterAudio(pJSON->d[_T("formats")], ydl_sd)) {
                stream.audio_url = ydl_sd.url;
                stream.video_url = _T("");
                streams.AddTail(stream);
            }
        }
    } else {
        if (pJSON->d.HasMember(_T("id")) && !pJSON->d[_T("id")].IsNull()) info.id = pJSON->d[_T("id")].GetString();
        if (pJSON->d.HasMember(_T("title")) && !pJSON->d[_T("title")].IsNull()) info.title = pJSON->d[_T("title")].GetString();
        if (pJSON->d.HasMember(_T("uploader")) && !pJSON->d[_T("uploader")].IsNull()) info.uploader = pJSON->d[_T("uploader")].GetString();
        if (pJSON->d.HasMember(_T("uploader_id")) && !pJSON->d[_T("uploader_id")].IsNull()) info.uploader_id = pJSON->d[_T("uploader_id")].GetString();
        if (pJSON->d.HasMember(_T("uploader_url")) && !pJSON->d[_T("uploader_url")].IsNull()) info.uploader_url = pJSON->d[_T("uploader_url")].GetString();
        if (pJSON->d.HasMember(_T("entries"))) {
            YDL_LOG(_T("Parsing playlist"));
            const Value& entries = pJSON->d[_T("entries")];

            for (rapidjson::SizeType i = 0; i < entries.Size(); i++) {
                YDL_LOG(_T("Playlist entry %d"), i);
                const Value& entry = entries[i];
                if ((!entry.HasMember(_T("formats")) || entry[_T("formats")].IsNull()) && (!entry.HasMember(_T("url")) || entry[_T("url")].IsNull())) {
                    continue;
                }
                if (filterVideo(entry, ydl_sd, s.iYDLMaxHeight, s.bYDLAudioOnly, s.iYDLVideoFormat)) {
                    stream.video_url = ydl_sd.url;
                    stream.audio_url = _T("");
                    if (entry.HasMember(_T("title")) && !entry[_T("title")].IsNull()) stream.title = entry[_T("title")].GetString();
                    if (entry.HasMember(_T("series")) && !entry[_T("series")].IsNull()) stream.series = entry[_T("series")].GetString();
                    if (entry.HasMember(_T("season")) && !entry[_T("season")].IsNull()) stream.season = entry[_T("season")].GetString();
                    if (entry.HasMember(_T("season_number")) && !entry[_T("season_number")].IsNull()) stream.season_number = entry[_T("season_number")].GetInt();
                    if (entry.HasMember(_T("season_id")) && !entry[_T("season_id")].IsNull()) stream.season_id = entry[_T("season_id")].GetString();
                    if (entry.HasMember(_T("episode")) && !entry[_T("episode")].IsNull()) stream.episode = entry[_T("episode")].GetString();
                    if (entry.HasMember(_T("episode_number")) && !entry[_T("episode_number")].IsNull()) stream.episode_number = entry[_T("episode_number")].GetInt();
                    if (entry.HasMember(_T("episode_id")) && !entry[_T("episode_id")].IsNull()) stream.episode_id = entry[_T("episode_id")].GetString();
                    if (entry.HasMember(_T("webpage_url")) && !entry[_T("webpage_url")].IsNull()) stream.webpage_url = entry[_T("webpage_url")].GetString();
                    if (ydl_sd.has_video && !ydl_sd.has_audio && entry.HasMember(_T("formats")) && !entry[_T("formats")].IsNull()) {
                        if (filterAudio(entry[_T("formats")], ydl_sd)) {
                            stream.audio_url = ydl_sd.url;
                        }
                    }
                    streams.AddTail(stream);
                } else if (entry.HasMember(_T("formats")) && !entry[_T("formats")].IsNull()) {
                    if (filterAudio(entry[_T("formats")], ydl_sd)) {
                        stream.audio_url = ydl_sd.url;
                        stream.title = entry[_T("title")].GetString();
                        stream.video_url = _T("");
                        streams.AddTail(stream);
                    }
                }
            }
        }
    }
    return !streams.IsEmpty();
}

bool CYoutubeDLInstance::loadJSON()
{
    if (!buf_out) {
        return false;
    }
    //the JSON buffer is ASCII with Unicode encoded with escape characters
    pJSON->d.Parse<rapidjson::kParseDefaultFlags, rapidjson::ASCII<>>(buf_out);
    if (pJSON->d.HasParseError()) {
        return false;
    }
    if (!pJSON->d.IsObject() || !pJSON->d.HasMember(_T("extractor"))) {
        return false;
    }
    bIsPlaylist = pJSON->d.FindMember(_T("entries")) != pJSON->d.MemberEnd();
    return true;
}
