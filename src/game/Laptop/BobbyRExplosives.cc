#include "Directories.h"
#include "Item_Types.h"
#include "Laptop.h"
#include "BobbyRExplosives.h"
#include "BobbyR.h"
#include "BobbyRGuns.h"
#include "Button_System.h"
#include "Object_Cache.h"
#include "Video.h"
#include "VSurface.h"


// The page of the Explosives of Bobby Ray's: the layout of the Misc page (its background and grid)
namespace {
cache_key_t const guiExplosivesBackground{ LAPTOPDIR "/miscbackground.sti" };
cache_key_t const guiExplosivesGrid{ LAPTOPDIR "/miscgrid.sti" };
}

void EnterBobbyRExplosives()
{
	InitBobbyBrTitle();
	//Draw menu bar
	InitBobbyMenuBar( );

	SetFirstLastPagesForNew( IC_EXPLOSV );

	RenderBobbyRExplosives( );
}


void ExitBobbyRExplosives()
{
	RemoveVObject(guiExplosivesBackground);
	RemoveVObject(guiExplosivesGrid);
	DeleteBobbyBrTitle();
	DeleteMouseRegionForBigImage();
	DeleteBobbyMenuBar();

	giCurrentSubPage = gusCurWeaponIndex;
	guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_EXPLOSIVES;
}


void RenderBobbyRExplosives()
{
	WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES,
		BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT,
		GetVObject(guiExplosivesBackground));

	//Display title at top of page
	DisplayBobbyRBrTitle();

	BltVideoObject(FRAME_BUFFER, guiExplosivesGrid, 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

	DisplayItemInfo(IC_EXPLOSV);

	UpdateButtonText(guiCurrentLaptopMode);
	MarkButtonsDirty( );
	RenderWWWProgramTitleBar( );
	InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
}
