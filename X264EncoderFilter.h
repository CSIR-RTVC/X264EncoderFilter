#pragma once
#include "VersionInfo.h"
#include <dshow.h>
#include <DirectShowExt/CustomBaseFilter.h>
#include <DirectShowExt/CodecControlInterface.h>
#include <DirectShowExt/DirectShowMediaFormats.h>
#include <DirectShowExt/FilterParameterStringConstants.h>
#include <DirectShowExt/ParameterConstants.h>
#include <x264.h>


// {AE822881-D2B3-426A-AC57-7EEBDF8D8789}
static const GUID CLSID_RTVC_X264Encoder =
{ 0xae822881, 0xd2b3, 0x426a,{ 0xac, 0x57, 0x7e, 0xeb, 0xdf, 0x8d, 0x87, 0x89 } };

// {265077D9-AE98-4356-9F3F-BFAC77CD7480}
static const GUID CLSID_X264Properties =
{ 0x265077d9, 0xae98, 0x4356,{ 0x9f, 0x3f, 0xbf, 0xac, 0x77, 0xcd, 0x74, 0x80 } };

#define FILTER_PARAM_PRESET "preset"
#define FILTER_PARAM_TUNE "tune"

#define TUNE_FILM "film"
#define TUNE_ANIMATION "animation"
#define TUNE_GRAIN "grain"
#define TUNE_STILLIMAGE "stillimage"
#define TUNE_PSNR "psnr"
#define TUNE_SSIM "ssim"
#define TUNE_FASTDECODE "fastdecode"
#define TUNE_ZEROLATENCY "zerolatency"
     
#define PRESET_ULTRAFAST "ultrafast"
#define PRESET_SUPERFAST "superfast"
#define PRESET_VERYFAST "veryfast"
#define PRESET_FASTER "faster"
#define PRESET_FAST "fast"
#define PRESET_MEDIUM "medium"
#define PRESET_SLOW "slow"
#define PRESET_SLOWER "slower"
#define PRESET_VERYSLOW "veryslow"


class X264EncoderFilter : public CCustomBaseFilter,
  public ISpecifyPropertyPages,
  public ICodecControlInterface
{
  std::vector<std::string> presets = { PRESET_ULTRAFAST, PRESET_SUPERFAST, PRESET_VERYFAST, PRESET_FASTER, PRESET_FAST, PRESET_MEDIUM, PRESET_SLOW, PRESET_SLOWER, PRESET_VERYSLOW };
  std::vector<std::string> tune_settings = { TUNE_FILM , TUNE_ANIMATION, TUNE_GRAIN, TUNE_STILLIMAGE, TUNE_PSNR, TUNE_SSIM, TUNE_FASTDECODE, TUNE_ZEROLATENCY };
public:
  DECLARE_IUNKNOWN

  /// Constructor
  X264EncoderFilter();
  /// Destructor
  ~X264EncoderFilter();

  /// Static object-creation method (for the class factory)
  static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr);

  /**
  * Overriding this so that we can set whether this is an RGB24 or an RGB32 Filter
  */
  HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);

  /**
  * Used for Media Type Negotiation
  * Returns an HRESULT value. Possible values include those shown in the following table.
  * <table border="0" cols="2"><tr valign="top"><td><b>Value</b></td><td><b>Description</b></td></TR><TR><TD>S_OK</TD><TD>Success</TD></TR><TR><TD>VFW_S_NO_MORE_ITEMS</TD><TD>Index out of range</TD></TR><TR><TD>E_INVALIDARG</TD><TD>Index less than zero</TD></TR></TABLE>
  * The output pin's CTransformOutputPin::GetMediaType method calls this method. The derived class must implement this method. For more information, see CBasePin::GetMediaType.
  * To use custom media types, the media type can be manipulated in this method.
  */
  HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

  /// Buffer Allocation
  /**
  * The output pin's CTransformOutputPin::DecideBufferSize method calls this method. The derived class must implement this method. For more information, see CBaseOutputPin::DecideBufferSize.
  * @param pAlloc Pointer to the IMemAllocator interface on the output pin's allocator.
  * @param pProp Pointer to an ALLOCATOR_PROPERTIES structure that contains buffer requirements from the downstream input pin.
  * @return Value: Returns S_OK or another HRESULT value.
  */
  HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);

  /**
  * The CheckTransform method checks whether an input media type is compatible with an output media type.
  * <table border="0" cols="2"> <tr valign="top"> <td  width="50%"><b>Value</b></td> <td width="50%"><b>Description</b></td> </tr> <tr valign="top"> <td width="50%">S_OK</td> <td width="50%">The media types are compatible.</td> </tr> <tr valign="top"> <td width="50%">VFW_E_TYPE_NOT_ACCEPTED</td> <td width="50%">The media types are not compatible.</td> </tr> </table>
  */
  HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);

  virtual void doGetVersion(std::string& sVersion)
  {
    sVersion = VersionInfo::toString();
  }
  /// Interface methods
  ///Overridden from CSettingsInterface
  virtual void initParameters()
  {
    //addParameter(D_FRAME_BIT_LIMIT, &m_nFrameBitLimit, 0);
    addParameter(CODEC_PARAM_IFRAME_PERIOD, &m_uiIFramePeriod, 25);
    addParameter(FILTER_PARAM_SPS, &m_sSeqParamSet, "", true);
    addParameter(FILTER_PARAM_PPS, &m_sPicParamSet, "", true);
    addParameter(FILTER_PARAM_H264_OUTPUT_TYPE, &m_nH264Type, H264_VPP);
    addParameter(FILTER_PARAM_TARGET_BITRATE_KBPS, &m_uiTargetBitrateKbps, 1500);
    addParameter(FILTER_PARAM_WIDTH, &m_nOutWidth, 0, true);
    addParameter(FILTER_PARAM_HEIGHT, &m_nOutHeight, 0, true);
    addParameter(FILTER_PARAM_TUNE, &m_sTune, TUNE_ZEROLATENCY);
    addParameter(FILTER_PARAM_PRESET, &m_sPreset, PRESET_ULTRAFAST);
  }

  /// Overridden from SettingsInterface
  STDMETHODIMP GetParameter(const char* szParamName, int nBufferSize, char* szValue, int* pLength);
  STDMETHODIMP SetParameter(const char* type, const char* value);
  STDMETHODIMP GetParameterSettings(char* szResult, int nSize);

  /**
  * @brief Overridden from ICodecControlInterface. Getter for frame bit limit
  */
  STDMETHODIMP GetFramebitLimit(int& iFrameBitLimit);
  /**
  * @brief @ICodecControlInterface. Overridden from ICodecControlInterface
  */
  STDMETHODIMP SetFramebitLimit(int iFrameBitLimit);
  /**
  * @brief @ICodecControlInterface. Method to query group id
  */
  STDMETHODIMP GetGroupId(int& iGroupId);
  /**
  * @brief @ICodecControlInterface. Method to set group id
  */
  STDMETHODIMP SetGroupId(int iGroupId);
  /**
  * @brief @ICodecControlInterface. Overridden from ICodecControlInterface
  */
  STDMETHODIMP GenerateIdr();
  STDMETHODIMP GetBitrateKbps(int& uiBitrateKbps);
  STDMETHODIMP SetBitrateKbps(int uiBitrateKbps);

  STDMETHODIMP GetPages(CAUUID *pPages)
  {
    if (pPages == NULL) return E_POINTER;
    pPages->cElems = 1;
    pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
      return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_X264Properties;
    return S_OK;
  }

  STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
  {
    if (riid == IID_ISpecifyPropertyPages)
    {
      return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
    }
    else if (riid == IID_ICodecControlInterface)
    {
      return GetInterface(static_cast<ICodecControlInterface*>(this), ppv);
    }
    else
    {
      // Call the parent class.
      return CCustomBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
  }

  /// Overridden from CCustomBaseFilter
  virtual void InitialiseInputTypes();


private:

  void reconfigure();
  /**
  This method copies the h.264 sequence and picture parameter sets into the passed in buffer
  and returns the total length including start codes
  */
  unsigned copySequenceAndPictureParameterSetsIntoBuffer(BYTE* pBuffer);
  unsigned getParameterSetLength() const;
  /**
   * This method converts the input buffer from RGB24 | 32 to YUV420P
   * @param pSource The source buffer
   * @param pDest The destination buffer
   */
  virtual HRESULT ApplyTransform(BYTE* pBufferIn, long lInBufferSize, long lActualDataLength, BYTE* pBufferOut, long lOutBufferSize, long& lOutActualDataLength);

  int m_nH264Type;
  ///// Receive Lock
  //CCritSec m_csCodec;
  //int m_nFrameBitLimit;
  //bool m_bNotifyOnIFrame;
  unsigned char* m_pSeqParamSet;
  unsigned m_uiSeqParamSetLen;
  unsigned char* m_pPicParamSet;
  unsigned m_uiPicParamSetLen;
  std::string m_sSeqParamSet;
  std::string m_sPicParamSet;

  //// For auto i-frame generation
  unsigned m_uiIFramePeriod;
  //// frame counter for i-frame generation
  //unsigned m_uiCurrentFrame;

  //REFERENCE_TIME m_rtFrameLength;
  //REFERENCE_TIME m_tStart;
  //REFERENCE_TIME m_tStop;

  x264_picture_t pic_in;
  x264_picture_t pic_out;
  x264_param_t params;
  x264_nal_t* nals;
  x264_t* encoder;
  int num_nals;

  uint32_t m_uiTargetBitrateKbps;

  uint32_t m_uiEncodingBufferSize;
  //rtp_plus_plus::Buffer m_encodingBuffer;

  uint32_t m_uiMode;
  double m_dCbrFactor;
  std::string m_sPreset;
  std::string m_sTune;

};
