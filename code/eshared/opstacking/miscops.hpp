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

#ifndef MISC_OPS_HPP
#define MISC_OPS_HPP

// collects sequencer operators and creates a demo
class eIDemoOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eDemo demo;
    };

public:
    eIDemoOp() :
        m_demo(m_res.demo),
        m_processAll(eFALSE)
    {
    }

    void setProcessAll(eBool processAll)
    {
        m_processAll = processAll;
    }

    eBool getProcessAll() const
    {
        return m_processAll;
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

protected:
    eDemo & m_demo;
    Result  m_res;
    eBool   m_processAll;
};

// base class for render-to-texture operator.
// can be used as texture in material operator.
class eIRenderToTextureOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eTexture2d * renderTarget;
        eTexture2d * depthTarget;
    };

public:
    eIRenderToTextureOp() :
        m_renderTarget(m_res.renderTarget),
        m_depthTarget(m_res.depthTarget)
    {
        m_renderTarget = nullptr;
        m_depthTarget = nullptr;
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

protected:
    Result         m_res;
    eTexture2d *&  m_renderTarget;
    eTexture2d *&  m_depthTarget;
};

// base class for material operator
class eIMaterialOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eMaterial mat;
    };

public:
    eIMaterialOp() : m_mat(m_res.mat)
    {
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

protected:
    eMaterial & m_mat;
    Result      m_res;
};

// operator between model and effect operators
class eISceneOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eScene scene; 
    };

public:
    eISceneOp() : m_scene(m_res.scene)
    {
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

protected:
    eScene & m_scene;
    Result   m_res;
};

// operator which holds a camera and a (relative) viewport
class eIPovOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eCamera  cam;
        eVector4 viewport;
    };

public:
    eIPovOp() :
        m_cam(m_res.cam),
        m_viewport(m_res.viewport)
    {
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

protected:
    eCamera &  m_cam;
    eVector4 & m_viewport;
    Result     m_res;
};

// base class for path operator
class eIPathOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        ePath4 path;
    };

public:
    eIPathOp() : m_path(m_res.path)
    {
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

protected:
    ePath4 & m_path;
    Result   m_res;
};

// base class for load, store and nop operators
class eIStructureOp : public eIOperator
{
public:
    virtual const eOpResult & getResult() const
    {
        return _getRealOp()->getResult();
    }

protected:
    virtual eIOperator * _getRealOp() const
    {
        return (m_aboveOps.size() > 0 ? m_aboveOps[0] : nullptr);
    }
};

// Generic operator.
class eIGenericOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        Result(void* &gd) : genericDataPtr(gd)
        {
        }

        (void*) & genericDataPtr;
    };

public:
    eIGenericOp() : m_result(m_genData)
    {
    }

    virtual const Result & getResult() const
    {
        return m_result;
    }

protected:
    void*   m_genData;
    Result  m_result;
};


#endif // MISC_OPS_HPP