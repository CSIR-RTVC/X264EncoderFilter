#include "stdafx.h"
#include "X264EncoderFilter.h"
#include <CodecUtils/CodecConfigurationUtil.h>
#include <CodecUtils/H264Util.h>
#include "Conversion.h"
#include <wmcodecdsp.h>
#include "Dvdmedia.h"

using namespace vpp::h264;

X264EncoderFilter::X264EncoderFilter()
  : CCustomBaseFilter(NAME("CSIR X264 Encoder"), 0, CLSID_RTVC_X264Encoder),
  nals(nullptr),
  encoder(nullptr),
  m_uiTargetBitrateKbps(300),
  m_uiIFramePeriod(25),
  m_uiEncodingBufferSize(0),
  m_uiMode(0),
  m_dCbrFactor(0.8),
  m_sPreset("ultrafast"),
  m_sTune("zerolatency"),
  m_nH264Type(H264_AVC1)
{
  InitialiseInputTypes();
  initParameters();
  // Set initial parameters base on the Preset and Tune
  x264_param_default_preset(&params, m_sPreset.c_str(), m_sTune.c_str());
}

X264EncoderFilter::~X264EncoderFilter()
{
  if (encoder) {
    x264_picture_clean(&pic_in);
    memset((char*)&pic_in, 0, sizeof(pic_in));
    memset((char*)&pic_out, 0, sizeof(pic_out));

    x264_encoder_close(encoder);
    encoder = NULL;
  }
}

void X264EncoderFilter::InitialiseInputTypes()
{
  //Setting the media types that passes the CheckMediaType
  AddInputType(&MEDIATYPE_Video, &MEDIASUBTYPE_I420, &FORMAT_VideoInfo);
  AddInputType(&MEDIATYPE_Video, &MEDIASUBTYPE_I420, &FORMAT_MPEG2Video);
}

CUnknown * WINAPI X264EncoderFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr)
{
  X264EncoderFilter *pFilter = new X264EncoderFilter();
  if (pFilter == NULL)
  {
    *pHr = E_OUTOFMEMORY;
  }
  return pFilter;
}

void X264EncoderFilter::reconfigure()
{
  if (m_nInWidth == 0 || m_nInHeight == 0) return;
  params.i_threads = 1;
  params.i_width = m_nInWidth;
  params.i_height = m_nInHeight;

  // we only handle I420 for now. Could insert a color converter here if we want to support RGB24
  //if (pmt->subtype == MEDIASUBTYPE_I420)
  {
    params.i_csp = X264_CSP_I420;
    x264_picture_alloc(&pic_in, X264_CSP_I420, m_nInWidth, m_nInHeight);
  }

  if (m_nH264Type == H264_VPP)
  {
    // Set startcode;
    params.b_annexb = 1;
  }
  else if (m_nH264Type == H264_AVC1)
  {
    params.b_annexb = 0;
  }

  if (m_videoInHeader.AvgTimePerFrame == 0)
  {
    params.i_fps_num = 25;
  }
  else
  {
    params.i_fps_num = static_cast<uint32_t>(10000000 / m_videoInHeader.AvgTimePerFrame);
  }

  params.i_fps_den = 1;

  //Set Buffersize base on the media sample
  m_uiEncodingBufferSize = static_cast<uint32_t>(m_nInWidth *m_nInHeight * 1.5);
  // for debugging
#if 0
  params.i_log_level = X264_LOG_DEBUG;
#endif
  params.i_keyint_max = m_uiIFramePeriod; //IDRPeriod
  params.rc.i_rc_method = X264_RC_ABR;
  params.rc.i_bitrate = m_uiTargetBitrateKbps;

  params.rc.i_vbv_buffer_size = m_uiTargetBitrateKbps;
  params.rc.i_vbv_max_bitrate = static_cast<int>(m_uiTargetBitrateKbps*m_dCbrFactor);
  params.rc.f_rate_tolerance = 1.0;

  //Making a copy of params before opening an encoder. (Work around the fact that params seems to be reseted after use)
  x264_param_t dummyParams = params;
  encoder = x264_encoder_open(&dummyParams);

  BYTE* pDummyFrameBuffer = new BYTE[m_uiEncodingBufferSize];
  memset(pDummyFrameBuffer, 0, m_uiEncodingBufferSize);
  memcpy(pic_in.img.plane[0], pDummyFrameBuffer, m_uiEncodingBufferSize);
  // Is this needed
  pic_in.img.i_stride[0] = m_nInWidth;
  pic_in.img.i_stride[1] = pic_in.img.i_stride[2] = m_nInWidth >> 1;
  int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);

  if (m_nH264Type == H264_VPP)
  {
    for (int i = 0; i < num_nals; ++i)
    {
      x264_nal_t * pNal = nals + i;
      int nalu_size = pNal->i_payload;
      BYTE startcode4[4] = { 0x00, 0x00 , 0x00 , 0x01 };
      BYTE startcode3[3] = { 0x00, 0x00, 0x01 };
      // Check the startcode before checking the isSps and isPps
      if (memcmp(pNal->p_payload, startcode4, 4) == 0)
      {
        if (isSps(pNal->p_payload[4]))
        {
          m_sSeqParamSet = std::string((const char*)pNal->p_payload, nalu_size);
          m_uiSeqParamSetLen = nalu_size;
        }
        else if (isPps(pNal->p_payload[4]))
        {
          m_sPicParamSet = std::string((const char*)pNal->p_payload, nalu_size);
          m_uiPicParamSetLen = nalu_size;
        }
      }
      else if (memcmp(pNal->p_payload, startcode3, 3) == 0)
      {
        if (isSps(pNal->p_payload[3]))
        {
          m_sSeqParamSet = std::string((const char*)pNal->p_payload, nalu_size);
          m_uiSeqParamSetLen = nalu_size;
        }
        else if (isPps(pNal->p_payload[3]))
        {
          m_sPicParamSet = std::string((const char*)pNal->p_payload, nalu_size);
          m_uiPicParamSetLen = nalu_size;
        }
      }
      else
      {
      }
    }
  }
  else if (m_nH264Type == H264_AVC1)
  {
    for (int i = 0; i < num_nals; ++i)
    {
      // Since startcode is replaced with 4 byte size. assume 4 byte before the sps and pps flag
      x264_nal_t * pNal = nals + i;
      int nalu_size = pNal->i_payload;
      //  can assume start code of SPS and PPS to have length 4
      if (isSps(pNal->p_payload[4]))
      {
        // need to convert 4 byte start code to 2 byte length prefix
        m_uiSeqParamSetLen = nalu_size - 2;
        m_sSeqParamSet = std::string((const char*)pNal->p_payload + 2, m_uiSeqParamSetLen);
      }
      else if (isPps(pNal->p_payload[4]))
      {
        // need to convert 4 byte start code to 2 byte length prefix
        m_uiPicParamSetLen = nalu_size - 2;
        m_sPicParamSet = std::string((const char*)pNal->p_payload + 2, m_uiPicParamSetLen);
      }
    }
  }
  x264_encoder_close(encoder);
  encoder = x264_encoder_open(&params);

  //Free the buffer
  delete pDummyFrameBuffer;
  pDummyFrameBuffer = nullptr;
}

HRESULT X264EncoderFilter::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
  HRESULT hr = CCustomBaseFilter::SetMediaType(direction, pmt);
  if (direction == PINDIR_INPUT)
  {
    if (pmt->formattype == FORMAT_VideoInfo)
    {
      //Load Preset and Tune in Constructor
      //x264_param_default_preset(&params, m_sPreset.c_str(), m_sTune.c_str());
      reconfigure();
    }
  }
  return hr;
}

HRESULT X264EncoderFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
  if (iPosition < 0)
  {
    return E_INVALIDARG;
  }
  else if (iPosition == 0)
  {
    if (m_nH264Type == H264_VPP)
    {
      // Get the input pin's media type and return this as the output media type - we want to retain
      // all the information about the image
      HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
      if (FAILED(hr))
      {
        return hr;
      }

      pMediaType->subtype = MEDIASUBTYPE_VPP_H264;
      ASSERT(pMediaType->formattype == FORMAT_VideoInfo);
      VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pMediaType->pbFormat;
      BITMAPINFOHEADER* pBi = &(pVih->bmiHeader);

      REFERENCE_TIME avgTimePerFrame = pVih->AvgTimePerFrame;
      // For compressed formats, this value is the implied bit 
      // depth of the uncompressed image, after the image has been decoded.
      if (pBi->biBitCount != 24)
        pBi->biBitCount = 24;

      pBi->biSizeImage = DIBSIZE(pVih->bmiHeader);
      pBi->biSizeImage = DIBSIZE(pVih->bmiHeader);

      // COMMENTING TO REMOVE
      // in the case of YUV I420 input to the H264 encoder, we need to change this back to RGB
      //pBi->biCompression = BI_RGB;
      pBi->biCompression = DWORD('1cva');

      // Store SPS and PPS in media format header
      int nCurrentFormatBlockSize = pMediaType->cbFormat;

      if (m_uiSeqParamSetLen + m_uiPicParamSetLen > 0)
      {
        // old size + one int to store size of SPS/PPS + SPS/PPS/prepended by start codes
        int iAdditionalLength = sizeof(int) + m_uiSeqParamSetLen + m_uiPicParamSetLen;
        int nNewSize = nCurrentFormatBlockSize + iAdditionalLength;
        pMediaType->ReallocFormatBuffer(nNewSize);

        BYTE* pFormat = pMediaType->Format();
        BYTE* pStartPos = &(pFormat[nCurrentFormatBlockSize]);
        // copy SPS
        memcpy(pStartPos, (void*)m_sSeqParamSet.c_str(), m_uiSeqParamSetLen);
        pStartPos += m_uiSeqParamSetLen;
        // copy PPS
        memcpy(pStartPos, (void*)m_sPicParamSet.c_str(), m_uiPicParamSetLen);
        pStartPos += m_uiPicParamSetLen;
        // Copy additional header size
        memcpy(pStartPos, &iAdditionalLength, sizeof(int));
      }
      else
      {
        // not configured: just copy in size of 0
        pMediaType->ReallocFormatBuffer(nCurrentFormatBlockSize + sizeof(int));
        BYTE* pFormat = pMediaType->Format();
        memset(pFormat + nCurrentFormatBlockSize, 0, sizeof(int));
      }
    }

    else if (m_nH264Type == H264_AVC1)
    {
      // Get the input pin's media type and return this as the output media type - we want to retain
      // all the information about the image
      HRESULT hr = m_pInput->ConnectionMediaType(pMediaType);
      if (FAILED(hr))
      {
        return hr;
      }
      VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pMediaType->pbFormat;
      REFERENCE_TIME avgTimePerFrame = pVih->AvgTimePerFrame;

      pMediaType->InitMediaType();
      pMediaType->SetType(&MEDIATYPE_Video);
      pMediaType->SetSubtype(&MEDIASUBTYPE_AVC1);
      pMediaType->SetFormatType(&FORMAT_MPEG2Video);

      ASSERT(m_uiSeqParamSetLen > 0 && m_uiPicParamSetLen > 0);

      // ps have start codes: for this media type we replace each start code with 2 byte lenght prefix
      int psLen = m_uiSeqParamSetLen + m_uiPicParamSetLen;
      BYTE* pFormatBuffer = pMediaType->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO) + psLen);
      MPEG2VIDEOINFO* pMpeg2Vih = (MPEG2VIDEOINFO*)pFormatBuffer;

      ZeroMemory(pMpeg2Vih, sizeof(MPEG2VIDEOINFO) + psLen);

      pMpeg2Vih->dwFlags = 4;
      pMpeg2Vih->dwProfile = 66;
      pMpeg2Vih->dwLevel = 20;

      pMpeg2Vih->cbSequenceHeader = psLen;
      int iCurPos = 0;

      BYTE* pSequenceHeader = (BYTE*)&pMpeg2Vih->dwSequenceHeader[0];
      // parameter set includes length
      memcpy(pSequenceHeader + iCurPos, (void*)m_sSeqParamSet.c_str(), m_uiSeqParamSetLen);
      iCurPos += m_uiSeqParamSetLen;
      memcpy(pSequenceHeader + iCurPos, (void*)m_sPicParamSet.c_str(), m_uiPicParamSetLen);

      VIDEOINFOHEADER2* pvi2 = &pMpeg2Vih->hdr;
      pvi2->bmiHeader.biBitCount = 24;
      pvi2->bmiHeader.biSize = 40;
      pvi2->bmiHeader.biPlanes = 1;
      pvi2->bmiHeader.biWidth = m_nInWidth;
      pvi2->bmiHeader.biHeight = m_nInHeight;
      pvi2->bmiHeader.biSizeImage = DIBSIZE(pvi2->bmiHeader);
      pvi2->bmiHeader.biCompression = DWORD('1cva');
      pvi2->AvgTimePerFrame = avgTimePerFrame;
      //SetRect(&pvi2->rcSource, 0, 0, m_cx, m_cy);
      SetRect(&pvi2->rcSource, 0, 0, m_nInWidth, m_nInHeight);
      pvi2->rcTarget = pvi2->rcSource;

      pvi2->dwPictAspectRatioX = m_nInWidth;
      pvi2->dwPictAspectRatioY = m_nInHeight;
    }
    return S_OK;
  }
  return VFW_S_NO_MORE_ITEMS;
}

HRESULT X264EncoderFilter::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
  AM_MEDIA_TYPE mt;
  HRESULT hr = m_pOutput->ConnectionMediaType(&mt);
  if (FAILED(hr))
  {
    return hr;
  }

  if (m_nH264Type == H264_VPP)
  {
    ASSERT(mt.formattype == FORMAT_VideoInfo);
    BITMAPINFOHEADER *pbmi = HEADER(mt.pbFormat);
    //TOREVISE: Should actually change mode and see what the maximum size is per frame?
    pProp->cbBuffer = DIBSIZE(*pbmi);
  }

  else if (m_nH264Type == H264_AVC1)
  {
    //Make sure that the format type is our custom format
    ASSERT(mt.formattype == FORMAT_MPEG2Video);
    MPEG2VIDEOINFO* pMpeg2Vih = (MPEG2VIDEOINFO*)mt.pbFormat;
    BITMAPINFOHEADER *pbmi = &pMpeg2Vih->hdr.bmiHeader;
    //TOREVISE: Should actually change mode and see what the maximum size is per frame?
    pProp->cbBuffer = DIBSIZE(*pbmi);
  }

  if (pProp->cbAlign == 0)
  {
    pProp->cbAlign = 1;
  }
  if (pProp->cBuffers == 0)
  {
    pProp->cBuffers = 1;
  }
  // Release the format block.
  FreeMediaType(mt);

  // Set allocator properties.
  ALLOCATOR_PROPERTIES Actual;
  hr = pAlloc->SetProperties(pProp, &Actual);
  if (FAILED(hr))
  {
    return hr;
  }
  // Even when it succeeds, check the actual result.
  if (pProp->cbBuffer > Actual.cbBuffer)
  {
    return E_FAIL;
  }
  return S_OK;
}

inline unsigned X264EncoderFilter::getParameterSetLength() const
{
  return m_uiSeqParamSetLen + m_uiPicParamSetLen;
}

inline unsigned X264EncoderFilter::copySequenceAndPictureParameterSetsIntoBuffer(BYTE* pBuffer)
{
  // The SPS and PPS contain start code
  memcpy(pBuffer, (void*)m_pSeqParamSet, m_uiSeqParamSetLen);
  pBuffer += m_uiSeqParamSetLen;
  memcpy(pBuffer, m_pPicParamSet, m_uiPicParamSetLen);
  pBuffer += m_uiPicParamSetLen;
  return getParameterSetLength();
}


HRESULT X264EncoderFilter::ApplyTransform(BYTE* pBufferIn, long lInBufferSize, long lActualDataLength, BYTE* pBufferOut, long lOutBufferSize, long& lOutActualDataLength)
{
  memcpy(pic_in.img.plane[0], pBufferIn, lInBufferSize);
  // Is this needed
  pic_in.img.i_stride[0] = m_nInWidth;
  pic_in.img.i_stride[1] = pic_in.img.i_stride[2] = m_nInWidth >> 1;
  int frame_size = x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out);
  if (frame_size > 0)
  {
    //Encoding was successful
    lOutActualDataLength = frame_size;
    BYTE* pBuffer = pBufferOut;
    for (int i = 0; i < num_nals; ++i)
    {
      x264_nal_t * pNal = nals + i;
      int nalu_size = pNal->i_payload;
      memcpy(pBuffer, nals[i].p_payload, nalu_size);
      pBuffer += nalu_size;
    }
  }
  else
  {
  }
  return S_OK;
}

HRESULT X264EncoderFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
  // Check the major type.
  if (mtOut->majortype != MEDIATYPE_Video)
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  if (m_nH264Type == H264_VPP)
  {
    if (mtOut->subtype != MEDIASUBTYPE_VPP_H264)
    {
      return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (mtOut->formattype != FORMAT_VideoInfo)
    {
      return VFW_E_TYPE_NOT_ACCEPTED;
    }
  }
  // Everything is good.
  return S_OK;
}

STDMETHODIMP X264EncoderFilter::GetParameter(const char* szParamName, int nBufferSize, char* szValue, int* pLength)
{
  if (SUCCEEDED(CCustomBaseFilter::GetParameter(szParamName, nBufferSize, szValue, pLength)))
  {
    return S_OK;
  }
  else
  {
    return E_FAIL;
  }
}

STDMETHODIMP X264EncoderFilter::GetBitrateKbps(int& uiBitrateKbps)
{
  uiBitrateKbps = m_uiTargetBitrateKbps;
  return S_OK;
}

STDMETHODIMP X264EncoderFilter::SetBitrateKbps(int uiBitrateKbps)
{
  if (uiBitrateKbps != m_uiTargetBitrateKbps)
  {
    x264_param_t param;
    x264_encoder_parameters(encoder, &param);
    // use re-config
    param.rc.i_bitrate = uiBitrateKbps;
    param.rc.i_vbv_buffer_size = uiBitrateKbps;
    param.rc.i_vbv_max_bitrate = static_cast<int>(uiBitrateKbps*m_dCbrFactor);
    int res = x264_encoder_reconfig(encoder, &param);
    if (res < 0)
    {
      return E_FAIL;
    }
    m_uiTargetBitrateKbps = uiBitrateKbps;
  }
  return S_OK;
}

STDMETHODIMP X264EncoderFilter::SetParameter(const char* type, const char* value)
{
  if (SUCCEEDED(CCustomBaseFilter::SetParameter(type, value)))
  {
    if ((std::string(type) == FILTER_PARAM_TARGET_BITRATE_KBPS) ||
      (std::string(type) == FILTER_PARAM_H264_OUTPUT_TYPE)
      )
    {
      reconfigure();
      return S_OK;
    }
    else if (std::string(type) == FILTER_PARAM_TUNE) {
      if (std::find(std::begin(tune_settings), std::end(tune_settings), std::string(value)) != tune_settings.end())
      {
        x264_param_default_preset(&params, m_sPreset.c_str(), m_sTune.c_str());
        reconfigure();
        return S_OK;
      }
      else
      {
        CCustomBaseFilter::revertParameter(type);
        return E_FAIL;
      }
    }
    else if (std::string(type) == FILTER_PARAM_PRESET) {
      if (std::find(std::begin(presets), std::end(presets), std::string(value)) != presets.end())
      {
        x264_param_default_preset(&params, m_sPreset.c_str(), m_sTune.c_str());
        reconfigure();
        return S_OK;
      }
      else
      {
        CCustomBaseFilter::revertParameter(type);
        return E_FAIL;
      }
    }
    else
    {
      return S_OK;
    }
  }
  else
  {
    return E_FAIL;
  }
}

STDMETHODIMP X264EncoderFilter::GetParameterSettings(char* szResult, int nSize)
{
  if (SUCCEEDED(CCustomBaseFilter::GetParameterSettings(szResult, nSize)))
  {
    return S_OK;
  }
  else
  {
    return E_FAIL;
  }
}


STDMETHODIMP X264EncoderFilter::GetFramebitLimit(int& iFrameBitLimit)
{
  return E_NOTIMPL;
}

STDMETHODIMP X264EncoderFilter::SetFramebitLimit(int iFrameBitLimit)
{
  return E_NOTIMPL;
}

STDMETHODIMP X264EncoderFilter::GetGroupId(int& iGroupId)
{
  return E_NOTIMPL;
}

STDMETHODIMP X264EncoderFilter::SetGroupId(int iGroupId)
{
  return E_NOTIMPL;
}

STDMETHODIMP X264EncoderFilter::GenerateIdr()
{
  return E_NOTIMPL;
}
