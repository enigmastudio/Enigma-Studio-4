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

#ifndef MODEL_OPS_HPP
#define MODEL_OPS_HPP

// base class for model operators
class eIModelOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        Result(eSceneData &sd) : sceneData(sd)
        {
        }

        eSceneData & sceneData;
    };

public:
    eIModelOp() : m_result(m_sceneData)
    {
    }

    virtual const Result & getResult() const
    {
        return m_result;
    }

private:
    virtual void _preExecute()
    {
        m_sceneData.clear();
    }

protected:
    eSceneData  m_sceneData;
    Result      m_result;
};

#endif // MODEL_OPS_HPP