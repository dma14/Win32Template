#pragma once
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <dwrite.h>
#pragma comment(lib, "dwrite")
#include <string>
#include <iostream>
#include "BaseWindow.h"
class MainWindow : public BaseWindow<MainWindow>
{
public:
	// Required override functions
	PCWSTR ClassName() const { return L"Drawing Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Constructor to initialize class-specific variables
	MainWindow(){}

private:
	// Direct2D setup variables
	// Direct2D factory, always needed
	ID2D1Factory* pFactory = NULL;
	// DirectWrite factory, used for text
	IDWriteFactory* pWriteFactory = NULL;
	IDWriteTextFormat* pTextFormat = NULL;
	// Render target representing the window, so we can draw on it
	ID2D1HwndRenderTarget* pRenderTarget = NULL;
	// Brush resource, used for drawing
	ID2D1SolidColorBrush* pBrush = NULL;
	// Ellipse object, the one we're drawing
	D2D1_ELLIPSE ellipse;
	// Timer ID pointer
	const UINT_PTR timerId = 1;
	// Counter for time elapsed, in 10ms increments
	unsigned long long timeElapsed = 0;

	// Function to create our Direct2D resources
	HRESULT CreateGraphicsResources();

	// Function to discard our Direct2D resources
	void DiscardGraphicsResources();

	// Function to handle the painting of our window
	void OnPaint();

	// Function to handle resizing of the window
	void Resize();

	// Function to update our own chapes
	void CalculateLayout();

	// Draw a single clock hand at the given angle
	void DrawClockHand(float handLength, float angle, float strokeWidth);
};

