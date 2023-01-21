
// Teleport (self) ----------------------------------------------------------

class ArtiTeleport : Inventory
{
	Default
	{
		+COUNTITEM
		+FLOATBOB
		+INVENTORY.INVBAR
		Inventory.PickupFlash "PickupFlash";
		+INVENTORY.FANCYPICKUPSOUND
		Inventory.DefMaxAmount;
		Inventory.Icon "ARTIATLP";
		Inventory.PickupSound "misc/p_pkup";
		Inventory.PickupMessage "$TXT_ARTITELEPORT";
		Tag "$TAG_ARTITELEPORT";
	}
	States
	{
	Spawn:
		ATLP ABCB 4;
		Loop;
	}
	
	override bool Use (bool pickup)
	{
		Vector3 dest;
		int destAngle;

		if (deathmatch)
		{
			[dest, destAngle] = level.PickDeathmatchStart();
		}
		else
		{
			[dest, destAngle] = level.PickPlayerStart(Owner.PlayerNumber());
		}
		if (!level.useplayerstartz)
			dest.Z = ONFLOORZ;
		Owner.Teleport (dest, destAngle, TELF_SOURCEFOG | TELF_DESTFOG);
		bool canlaugh = true;
		Playerinfo p = Owner.player;
		if (p && p.morphTics && (p.MorphStyle & MRF_UNDOBYCHAOSDEVICE))
		{ // Teleporting away will undo any morph effects (pig)
			if (!p.mo.UndoPlayerMorph (p, MRF_UNDOBYCHAOSDEVICE) && (p.MorphStyle & MRF_FAILNOLAUGH))
			{
				canlaugh = false;
			}
		}
		if (canlaugh)
		{ // Full volume laugh
			Owner.A_StartSound ("*evillaugh", CHAN_VOICE, CHANF_DEFAULT, 1., ATTN_NONE);
		}
		return true;
	}
	
}


