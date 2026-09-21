#include "Directories.h"
#include "Item_Types.h"
#include "Laptop.h"
#include "BobbyRAttachments.h"
#include "BobbyR.h"
#include "BobbyRGuns.h"
#include "Button_System.h"
#include "Object_Cache.h"
#include "Video.h"
#include "VSurface.h"


// The page of the Attachments of Bobby Ray's: the layout of the Misc page (its background and grid)
namespace {
cache_key_t const guiAttachmentsBackground{ LAPTOPDIR "/miscbackground.sti" };
cache_key_t const guiAttachmentsGrid{ LAPTOPDIR "/miscgrid.sti" };
}

void EnterBobbyRAttachments()
{
	InitBobbyBrTitle();
	//Draw menu bar
	InitBobbyMenuBar( );

	SetFirstLastPagesForNew( BobbyRAttachmentsPageMask() );

	RenderBobbyRAttachments( );
}


void ExitBobbyRAttachments()
{
	RemoveVObject(guiAttachmentsBackground);
	RemoveVObject(guiAttachmentsGrid);
	DeleteBobbyBrTitle();
	DeleteMouseRegionForBigImage();
	DeleteBobbyMenuBar();

	giCurrentSubPage = gusCurWeaponIndex;
	guiLastBobbyRayPage = LAPTOP_MODE_BOBBY_R_ATTACHMENTS;
}


void RenderBobbyRAttachments()
{
	WebPageTileBackground(BOBBYR_NUM_HORIZONTAL_TILES, BOBBYR_NUM_VERTICAL_TILES,
		BOBBYR_BACKGROUND_WIDTH, BOBBYR_BACKGROUND_HEIGHT,
		GetVObject(guiAttachmentsBackground));

	//Display title at top of page
	DisplayBobbyRBrTitle();

	BltVideoObject(FRAME_BUFFER, guiAttachmentsGrid, 0, BOBBYR_GRIDLOC_X, BOBBYR_GRIDLOC_Y);

	DisplayItemInfo(BobbyRAttachmentsPageMask());

	UpdateButtonText(guiCurrentLaptopMode);
	MarkButtonsDirty( );
	RenderWWWProgramTitleBar( );
	InvalidateRegion(LAPTOP_SCREEN_UL_X,LAPTOP_SCREEN_WEB_UL_Y,LAPTOP_SCREEN_LR_X,LAPTOP_SCREEN_WEB_LR_Y);
}
