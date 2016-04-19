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

#ifndef SEQUENCER_OPS_HPP
#define SEQUENCER_OPS_HPP

class eISequencerOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eSequencer seq;
    };

public:
    eISequencerOp() : m_seq(m_res.seq)
    {
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

private:
    virtual void _preExecute()
    {
        m_seq.clear();
    }

protected:
    eSequencer & m_seq;
    Result       m_res;
};

#endif // SEQUENCER_OPS_HPP