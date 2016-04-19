/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       ______        _                             __ __
 *      / ____/____   (_)____ _ ____ ___   ____ _   / // /
 *     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
 *    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
 *   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
 *                    /____/                              
 *
 *   Copyright © 2003-2012 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef BITMAP_OPS_HPP
#define BITMAP_OPS_HPP

// base class for bitmap operators
class eIBitmapOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eU32                width;
        eU32                height;
        eUavBuffer *        uav;
    };

public:
    eIBitmapOp();
    virtual ~eIBitmapOp();

    virtual const Result &  getResult() const;
    virtual eU32            getResultSize() const;
    virtual void            freeResult();

private:
    virtual void            _preExecute();

protected:
    void                    _reallocate(eU32 width, eU32 height);
    void                    _allocateUav(eU32 width, eU32 height, eUavBuffer *&uav) const;

    void                    _execCs(eComputeShader *cs, eInt texFlags0=eTMF_BILINEAR|eTMF_CLAMP,
                                    eUavBuffer *dstUav=nullptr, eUavBuffer *srcUav=nullptr,
                                    eIConstBuffer *cbExtra=nullptr, const eSize &threadDim=eSize(0, 0));
protected:
    eUavBuffer *&           m_uav;
    eU32 &                  m_bmpWidth;
    eU32 &                  m_bmpHeight;
    Result                  m_res;
};

#endif // BITMAP_OPS_HPP