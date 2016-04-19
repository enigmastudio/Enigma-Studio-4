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

#include "tfvsti.hpp"
#include <windows.h>

eTfEditor::eTfEditor (AudioEffect *effect) : AEffEditor (effect)
{
    QApplication::setWheelScrollLines(1);
    window = nullptr;
    effect->setEditor(this);
}

eTfEditor::~eTfEditor()
{
	eDelete(window);
}

void clientResize(HWND h_parent, int width, int height)
{
	RECT rcClient, rcWindow;
	POINT ptDiff;
	GetClientRect(h_parent, &rcClient);
	GetWindowRect(h_parent, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(h_parent, rcWindow.left, rcWindow.top, width + ptDiff.x, height + ptDiff.y, TRUE);
}

long eTfEditor::open (void *ptr)
{
	AEffEditor::open (ptr);

    eU32 width = 1080;
    eU32 height = 720;
       
	window = new tfWindow(effect, static_cast<HWND>(ptr));
	window->move( 0, 0 );
	window->setMinimumSize(width, height);
	window->adjustSize();
	rectangle.top = 0;
	rectangle.left = 0;
	rectangle.bottom = window->height();
	rectangle.right = window->width();
	window->setMinimumSize(window->size());
	window->show();

	//clientResize(static_cast<HWND>(ptr), window->width(), window->height());
 
	return true;
}

bool eTfEditor::keysRequired()
{
    return true;
}

long eTfEditor::onKeyDown (VstKeyCode &keyCode)
{
	return -1;
}

long eTfEditor::onKeyUp(VstKeyCode &keyCode)
{
	return -1; 
}

long eTfEditor::getRect(ERect** rect)
{
	*rect = &rectangle;
	return 1;
}

void eTfEditor::close()
{
	eDelete(window);
}

void eTfEditor::idle()
{
    QApplication::processEvents();
}

void eTfEditor::setParameter(long index, float value)
{
    postUpdate();
	// call this to be sure that the graphic will be updated
}

