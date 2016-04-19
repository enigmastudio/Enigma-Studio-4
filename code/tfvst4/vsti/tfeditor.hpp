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

#include "../vstsdk/AEffEditor.hpp"

//-----------------------------------------------------------------------------
class eTfEditor : public AEffEditor
{
public:
	eTfEditor (AudioEffect *effect);
	virtual ~eTfEditor ();

	void                suspend ();
	void                resume ();
	bool                keysRequired ();
	void                setParameter (long index, float value);
    void                idle();

protected:
	virtual long        open (void *ptr);
	virtual void        close ();
	virtual long		getRect (ERect** rect);

	// VST 2.1
	virtual long        onKeyDown (VstKeyCode &keyCode);
	virtual long        onKeyUp (VstKeyCode &keyCode);

private:
    tfWindow *window;
	ERect rectangle;
};

