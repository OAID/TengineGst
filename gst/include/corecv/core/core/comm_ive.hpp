#ifndef __COMM_IVE_H__
#define __COMM_IVE_H__

#ifdef CV_USE_IVE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

// HiSilicon
#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vgs.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_ive.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vgs.h"
#include "mpi_ive.h"

#define IVE_ALIGN 16
#define IVE_CHAR_CALW 8
#define IVE_CHAR_CALH 8
#define IVE_CHAR_NUM (IVE_CHAR_CALW *IVE_CHAR_CALH)
#define IVE_FILE_NAME_LEN 256
#define IVE_RECT_NUM   20

#define IVE_ALIGN_BACK(x, a)     ((a) * (((x) / (a))))

typedef struct hiSAMPLE_IVE_SWITCH_S
{
   HI_BOOL bVenc;
   HI_BOOL bVo;
}IVE_SWITCH_S;

typedef struct hiSAMPLE_IVE_RECT_S
{
    POINT_S astPoint[4];
} IVE_RECT_S;

typedef struct hiSAMPLE_RECT_ARRAY_S
{
    HI_U16 u16Num;
    IVE_RECT_S astRect[IVE_RECT_NUM];
} IVE_RECT_ARRAY_S;

typedef struct hiIVE_LINEAR_DATA_S
{
    HI_S32 s32LinearNum;
    HI_S32 s32ThreshNum;
    POINT_S* pstLinearPoint;
} IVE_LINEAR_DATA_S;

typedef struct hiSAMPLE_IVE_DRAW_RECT_MSG_S
{
    VIDEO_FRAME_INFO_S stFrameInfo;
    IVE_RECT_ARRAY_S stRegion;
} IVE_DRAW_RECT_MSG_S;

//free mmz
#define IVE_MMZ_FREE(phy,vir)\
    do{\
        if ((0 != (phy)) && (0 != (vir)))\
        {\
            HI_MPI_SYS_MmzFree((phy),(HI_VOID *)(HI_UL)(vir));\
            (phy) = 0;\
            (vir) = 0;\
        }\
    }while(0)

#define IVE_CLOSE_FILE(fp)\
    do{\
        if (NULL != (fp))\
        {\
            fclose((fp));\
            (fp) = NULL;\
        }\
    }while(0)

#define IVE_PAUSE()\
    do {\
        printf("---------------press Enter key to exit!---------------\n");\
        (void)getchar();\
    } while (0)
#define IVE_CHECK_EXPR_RET(expr, ret, fmt...)\
do\
{\
    if(expr)\
    {\
        fprintf(stderr, fmt);\
        return (ret);\
    }\
}while(0)
#define IVE_CHECK_EXPR_GOTO(expr, label, fmt...)\
do\
{\
    if(expr)\
    {\
        fprintf(stderr, fmt);\
        goto label;\
    }\
}while(0)

#define COMM_IVE_CONVERT_64BIT_ADDR(Type,Addr) (Type*)(HI_UL)(Addr)

/******************************************************************************
* function : Mpi init
******************************************************************************/
HI_VOID COMM_IVE_CheckIveMpiInit(HI_VOID);
/******************************************************************************
* function : Mpi exit
******************************************************************************/
HI_S32 COMM_IVE_IveMpiExit(HI_VOID);
/******************************************************************************
* function :Calc stride
******************************************************************************/
HI_U16 COMM_IVE_CalcStride(HI_U32 u32Width, HI_U8 u8Align);
/******************************************************************************
* function : Copy blob to rect
******************************************************************************/
HI_VOID COMM_IVE_BlobToRect(IVE_CCBLOB_S *pstBlob, IVE_RECT_ARRAY_S *pstRect,
                                            HI_U16 u16RectMaxNum,HI_U16 u16AreaThrStep,
                                            HI_U32 u32SrcWidth, HI_U32 u32SrcHeight,
                                            HI_U32 u32DstWidth,HI_U32 u32DstHeight);
/******************************************************************************
* function : Create ive image
******************************************************************************/
HI_S32 COMM_IVE_CreateImage(IVE_IMAGE_S* pstImg, IVE_IMAGE_TYPE_E enType,
                                   HI_U32 u32Width, HI_U32 u32Height);
/******************************************************************************
* function : Create memory info
******************************************************************************/
HI_S32 COMM_IVE_CreateMemInfo(IVE_MEM_INFO_S* pstMemInfo, HI_U32 u32Size);
/******************************************************************************
* function : Create ive image from CoreCV image
******************************************************************************/
HI_S32 COMM_IVE_CV2IVE(IVE_IMAGE_S* pstImg, const cv::Mat& cvImg);
HI_S32 COMM_IVE_CV2IVE_COPY(IVE_IMAGE_S* pstImg, const cv::Mat& cvImg);
/******************************************************************************
* function : Create CoreCV image from ive image
******************************************************************************/
HI_S32 COMM_IVE_IVE2CV(cv::Mat& cvImg, IVE_IMAGE_S* pstImg);

#endif // USE_IVE

#endif // __COMM_IVE_H__