/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_API_DEF_H_
#define _AX_OPAL_API_DEF_H_

#include "ax_opal_mal_pipeline.h"

AX_OPAL_PPL_ATTR_T g_stPplAttr = {
    .nSubPplCnt = 2,
    .arrSubPplAttr = {
        [0] = {
            .nId = 0,
            .nGrpCnt = 1,
            .arrGrpAttr = {
                [0] = {
                    .nGrpId = 0,
                    .nChnCnt = 3,
                    .nChnId = {0, 1, 2},
                },
            },
            .nEleCnt = 5,
            /* cam, ivps, main venc, detect, sub venc */
            .arrEleAttr = {
                [0] = {
                    .nId = 0,
                    .eType = AX_OPAL_ELE_CAM,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 1,
                            .nChnId = {0},
                        },
                    },
                },
                [1] = {
                    .nId = 1,
                    .eType = AX_OPAL_ELE_IVPS,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 3,
                            .nChnId = {0, 1, 2},
                        },
                    },
                },
                [2] = {
                    .nId = 2,
                    .eType = AX_OPAL_ELE_VENC,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 1,
                            .nChnId = {0},
                        },
                    },
                },
                [3] = {
                    .nId = 3,
                    .eType = AX_OPAL_ELE_ALGO,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 1,
                            .nChnId = {0},
                        },
                    },
                },
                [4] = {
                    .nId = 4,
                    .eType = AX_OPAL_ELE_VENC,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 1,
                            .nChnId = {1},
                        },
                    },
                },
            },
            .nLinkCnt = 8,
            .arrLinkAttr = {
                [0] = {AX_OPAL_SUBPPL, 0, 0, 0, AX_OPAL_ELE_CAM, 0, 0, 0, AX_OPAL_SUBPPL_IN},
                [1] = {AX_OPAL_ELE_CAM, 0, 0, 0, AX_OPAL_ELE_IVPS, 1, 0, 0, AX_OPAL_ELE_LINK},
                [2] = {AX_OPAL_ELE_IVPS, 1, 0, 0, AX_OPAL_ELE_VENC, 2, 0, 0, AX_OPAL_ELE_LINK},
                [3] = {AX_OPAL_ELE_IVPS, 1, 0, 1, AX_OPAL_ELE_ALGO, 3, 0, 0, AX_OPAL_ELE_NONLINK_FRM},
                [4] = {AX_OPAL_ELE_IVPS, 1, 0, 2, AX_OPAL_ELE_VENC, 4, 0, 1, AX_OPAL_ELE_LINK},
                [5] = {AX_OPAL_ELE_VENC, 2, 0, 0, AX_OPAL_SUBPPL, 0, 0, 0, AX_OPAL_SUBPPL_OUT},
                [6] = {AX_OPAL_ELE_ALGO, 3, 0, 0, AX_OPAL_SUBPPL, 0, 0, 1, AX_OPAL_SUBPPL_OUT},
                [7] = {AX_OPAL_ELE_VENC, 4, 0, 1, AX_OPAL_SUBPPL, 0, 0, 2, AX_OPAL_SUBPPL_OUT},
            },
        },
        [1] = {
            .nId = 1,
            .nGrpCnt = 1,
            .arrGrpAttr = {
                [0] = {
                    .nGrpId = 1,
                    .nChnCnt = 3,
                    .nChnId = {0, 1, 2},
                },
            },
            .nEleCnt = 5,
            /* cam, ivps, main venc, detect, sub venc */
            .arrEleAttr = {
                [0] = {
                    .nId = 0,
                    .eType = AX_OPAL_ELE_CAM,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 1,
                            .nChnCnt = 1,
                            .nChnId = {0},
                        },
                    },
                },
                [1] = {
                    .nId = 1,
                    .eType = AX_OPAL_ELE_IVPS,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 1,
                            .nChnCnt = 3,
                            .nChnId = {0, 1, 2},
                        },
                    },
                },
                [2] = {
                    .nId = 2,
                    .eType = AX_OPAL_ELE_VENC,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 1,
                            .nChnId = {2},
                        },
                    },
                },
                [3] = {
                    .nId = 3,
                    .eType = AX_OPAL_ELE_ALGO,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 1,
                            .nChnId = {1},
                        },
                    },
                },
                [4] = {
                    .nId = 4,
                    .eType = AX_OPAL_ELE_VENC,
                    .nGrpCnt = 1,
                    .arrGrpAttr = {
                        [0] = {
                            .nGrpId = 0,
                            .nChnCnt = 1,
                            .nChnId = {3},
                        },
                    },
                },
            },
            .nLinkCnt = 8,
            .arrLinkAttr = {
                [0] = {AX_OPAL_SUBPPL, 1, 1, 0, AX_OPAL_ELE_CAM, 0, 1, 0, AX_OPAL_SUBPPL_IN},
                [1] = {AX_OPAL_ELE_CAM, 0, 1, 0, AX_OPAL_ELE_IVPS, 1, 1, 0, AX_OPAL_ELE_LINK},
                [2] = {AX_OPAL_ELE_IVPS, 1, 1, 0, AX_OPAL_ELE_VENC, 2, 0, 2, AX_OPAL_ELE_LINK},
                [3] = {AX_OPAL_ELE_IVPS, 1, 1, 1, AX_OPAL_ELE_ALGO, 3, 0, 1, AX_OPAL_ELE_NONLINK_FRM},
                [4] = {AX_OPAL_ELE_IVPS, 1, 1, 2, AX_OPAL_ELE_VENC, 4, 0, 3, AX_OPAL_ELE_LINK},
                [5] = {AX_OPAL_ELE_VENC, 2, 0, 2, AX_OPAL_SUBPPL, 1, 1, 0, AX_OPAL_SUBPPL_OUT},
                [6] = {AX_OPAL_ELE_ALGO, 3, 0, 1, AX_OPAL_SUBPPL, 1, 1, 1, AX_OPAL_SUBPPL_OUT},
                [7] = {AX_OPAL_ELE_VENC, 4, 0, 3, AX_OPAL_SUBPPL, 1, 1, 2, AX_OPAL_SUBPPL_OUT},
            },
        },
    },
};

#endif // _AX_OPAL_API_DEF_H_