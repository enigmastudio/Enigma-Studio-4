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

#ifndef EFFECT_OPS_HPP
#define EFFECT_OPS_HPP

// base class for post effects
class eIEffectOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eIEffect *  fx;
    };

public:
    eIEffectOp() : m_rootFx(m_res.fx)
    {
        m_rootFx = nullptr;
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

private:
    virtual void _preExecute()
    {
        if (getAboveOpCount() > 0)
            if (getAboveOp(0)->getResultClass() == eOC_FX)
                m_rootFx = ((eIEffectOp *)getAboveOp(0))->getResult().fx;
    }

protected:
    void _appendEffect(eIEffect *fx)
    {
        fx->addInput(m_rootFx);
        m_rootFx = fx;
    }

protected:
    Result      m_res;
    eIEffect *& m_rootFx;
};

#endif // EFFECT_OPS_HPP