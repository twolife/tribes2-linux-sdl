// For T2 Linux: Patch for "broken" strcmp function.
// In Windows, that function delivers a direction and magnitude. In Linux, that function delivers a direction only.

// Patch by Electricutioner

package strCmpPatch
{
	function strcmp(%a, %b)
	{
		%posA = 0;
		if (%a !$= "")
			%posA = strPos($Patch::Strcmp::ASCIIStream, %a) + 1;
		%posB = 0;
		if (%b !$= "")
			%posB = strPos($Patch::Strcmp::ASCIIStream, %b) + 1;
		return (%posA - %posB);
	}

	function generateASCIIStream()
	{
		$Patch::Strcmp::ASCIIStream = "";
		for (%i = 1; %i <= 255; %i++)
		{
			$Patch::Strcmp::ASCIIStream = $Patch::Strcmp::ASCIIStream @ collapseEscape("\\x" @ DecToHex(%i));
		}
	}
};

// Bone functions
function DecToBin(%dec)
{
	%length = mCeil(mLog(%dec) / mLog(2));
	%bin = "";
	for (%i = 0; %i <= %length; %i++)
	{
		%test = mPow(2, %length - %i);
		if (%dec >= %test)
		{
			%bin = %bin @ "1";
			%dec -= %test;
		}
		else if (%i > 0)
			%bin = %bin @ "0";
	}
	return %bin;
}
function BinToDec(%bin)
{
	%dec = 0;
	for (%i = 0; %i < strLen(%bin); %i++)
		%dec += getSubStr(%bin, %i, 1) * mPow(2, strLen(%bin) - %i - 1);
	return %dec;
}
function DecToHex(%dec)
{
	%bin = DecToBin(%dec);
	while (strLen(%bin) % 4 != 0)
		%bin = "0" @ %bin;

	for (%i = 0; %i < strLen(%bin); %i += 4)
	{
		%block = getSubStr(%bin, strLen(%bin) - %i - 4, 4);
		%part = BinToDec(%block);
		if (%part > 9)
		{
			switch (%part)
			{
				case 10:
					%hex = "a" @ %hex;
				case 11:
					%hex = "b" @ %hex;
				case 12:
					%hex = "c" @ %hex;
				case 13:
					%hex = "d" @ %hex;
				case 14:
					%hex = "e" @ %hex;
				case 15:
					%hex = "f" @ %hex;
			}
		}
		else
			%hex = %part @ %hex;
	}

	//special purpose modification:
	//pad out the hex output to lengh that is divisible by 2 and is not zero
	while (strLen(%hex) == 0 || strLen(%hex) % 2 != 0)
		%hex = "0" @ %hex;
	return %hex;
}

if ($platform $= "Linux")
{
	activatePackage(strCmpPatch);
	generateASCIIStream();
}

