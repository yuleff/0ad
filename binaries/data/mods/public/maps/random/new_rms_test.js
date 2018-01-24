Engine.LoadLibrary("rmgen");

var g_Map = new RandomMap(0, "grass1_spring");

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.39))
});

placePlayersNomad(createTileClass());

g_Map.ExportMap();
