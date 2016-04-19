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

#ifndef DEMO_HPP
#define DEMO_HPP

class eDemo
{
public:
    eDemo();
    ~eDemo();

    eSeqResult              render(eF32 time) const;
    void                    setSequencer(const eSequencer *seq);
    const eSequencer *      getSequencer() const;

private:
    void                    _setupTarget(eF32 aspectRatio) const;

public:
    static const eF32       MAX_RUNNING_TIME_MINS;

private:
    const eSequencer *      m_seq;
    mutable eTexture2d *    m_target;
	mutable eTexture2d *	m_depthTarget;
    mutable eRect           m_renderRect;
};

#endif // DEMO_HPP