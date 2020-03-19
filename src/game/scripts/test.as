class TestCommand : ICommand
{
	private int a;
	private int b;

	TestCommand(int a, int b)
	{
		Log("Instantiating TestCommand.\n");
		this.a = a;
		this.b = b;
	}

	int opCall()
	{
		int sum = a + b;
		game_server.SendToAll("Ran TestCommand. Result: " + sum);
		return 0;
	}
};

interface TestInterface
{
	void TestInterfaceMethod();
};

class Meower : TestInterface, IPersistent
{
	uuid m_uuid;
	string m_name;

	void Load(uuid key)
	{

	}

	void DefineSchema(DBTable@ table)
	{

	}

	void Save()
	{
		Log("Saving meower.\n");
	}

	void TestInterfaceMethod()
	{
		Log("Calling Meower's testinterface method.\n");
	}

	Meower()
	{
		Log("Meower constructing.\n");
		TestInterfaceMethod();
		m_name = "Meower";
		m_uuid.Generate();
		Log("Trying to make a meower. это - кошка!\n");
		Log("Meower uuid: " + m_uuid.ToString() + "\n");
		Meower::Save();
	}
	~Meower()
	{

	}
	string GetUUID()
	{
		return m_uuid.ToString();
	}
};

class SuperMeower : Meower
{
	string m_superpowername;

	void TestInterfaceMethod()
	{
		Log("Calling SuperMeower's testinterface method.\n");
		Meower::TestInterfaceMethod();
	}
	SuperMeower()
	{
		Log("SuperMeower constructing.\n");
		TestInterfaceMethod();
		Save();
		m_superpowername = "meowing";
	}
	void Save()
	{

	}

};

enum PlayerGameState {LOGIN_MENU = 0,
ACCOUNT_NAME_ENTRY, ACCOUNT_PASSWORD_ENTRY };
class Player : PlayerConnection
{

	Player()
	{
		super();
		m_gamestate = PlayerGameState::LOGIN_MENU;

	}
	~Player()
	{
	}

	PlayerGameState GetPlayerGameState()
	{
		return m_gamestate;
	}

	void SetPlayerGameState(PlayerGameState v)
	{
		m_gamestate = v;
	}

	string GetName()
	{
		return m_name;
	}

	void SetName(string v)
	{
		m_name = v;
	}

	Meower@ GetMeower() { return m_meower;}
	void SetMeower(Meower@ m)
	{
		@m_meower = @m;
		Log("Attempting to set player meower.\n");
		Send("Trying to set your meower. это - кошка. \r\n");
		string msg = "\r\nYour meower's UUID is `@ff0000`" + m_meower.GetUUID() + "`default`!\r\n";
		Send(msg);
	}

	private PlayerGameState m_gamestate;
	private string m_name;
	private Meower@ m_meower;
};

TestCommand tc(1, 2);
Player@ hPlayer = null;
array<Player@> g_players;
array<Meower@> g_meowers;
void GameTick()
{
/*
  game_server.SendToAll("`red`Hello!`default`");
  game_server.QueueScriptCommand(tc, 4);
  game_server.QueueScriptCommand(tc, 2);
  if(hPlayer !is null)
  {
  hPlayer.Send("Ticking for player\r\n");
  }
  else
  {
  Log("There are no players connected.\r\n");
  }
*/
}



void OnPlayerConnect(Player@ player)
{
//	try
	{
	//ref@ h = @player;
	player.Send("Hello!\r\n");
	player.Send("Account: ");
	uuid newuuid;
	newuuid.Generate();
	player.Send("\r\n" + newuuid.ToString() + "\r\n");
	g_meowers.insertLast(SuperMeower());
	player.SetMeower(g_meowers[g_meowers.length() - 1]);
	g_players.insertLast(player);

	game_server.SendToAll("\r\nSomeone has connected. There are " + g_players.length() + " players connected.\r\n");
	}
//	catch
//	{
//		player.Send("Exception thrown!\r\n" + getExceptionInfo() + "\r\n");
//	}
/*
  Log("OnPlayerConnect()");
  player.Send("WELCOME!\r\n");

  @hPlayer = @player;
  if(hPlayer !is null)
  {
  hPlayer.Send("Handle assigned.\r\n");
  }
*/
}

void OnPlayerDisconnect(Player@ player)
{
	Log("OnPlayerDisconnect called.\n");
	int removeidx = g_players.findByRef(player);
	if(removeidx >= 0)
	{
		Log("Attempting to remove player at idx " + removeidx);
		int meoweridx = g_meowers.findByRef(player.GetMeower());
		game_server.SendToAll("g_meowers size: " + g_meowers.length() + "\r\n");
		g_meowers.removeAt(meoweridx);
		game_server.SendToAll("g_meowers size after removal: " + g_meowers.length() + "\r\n");
		g_players.removeAt(removeidx);
		game_server.SendToAll("Someone has disconnected. There are now " + g_players.length() + " players connected.\r\n");
	}
	else
	{
		Log("Couldn't find player in g_players.\n");
	}
/*
  Log("Someone disconnected.\r\n");
  if(@hPlayer is @player)
  {
  @hPlayer = null;
  }
*/
}

void TestDatabase(Player@ player)
{
	DBTable testtable("testtable");
	testtable.AddUUIDColumn("uuid", COLKEYTYPE_PRIMARYKEY);
	testtable.AddTextColumn("name");
	DBRow@ testrow = testtable.MakeRow();
	uuid testuuid;
	testuuid.Generate();
	testrow.SetColValue("uuid", testuuid);
	testrow.SetColValue("name", "meower");
	testrow.StoreIntoDB();
	string uuidstr = testuuid.ToString();
	player.Send("Generated guid: " + uuidstr + "\r\n");
	uuid otheruuid;
	otheruuid.FromString(uuidstr);
	player.Send("Converted guid: " + otheruuid.ToString() + "\r\n");
}

void TestDatabaseRead(Player@ player)
{
	DBTable testtable("testtable");
	testtable.AddUUIDColumn("uuid", COLKEYTYPE_PRIMARYKEY);
	testtable.AddTextColumn("name");
	DBRow@ testrow = testtable.MakeRow();
	uuid keyuuid;
	keyuuid.FromString("0e6e8006-698a-46ec-8aa4-211fa6a1892c");
	testrow.SetColValue("uuid", keyuuid);
	testrow.LoadFromDB();
	string name;
	testrow.GetColValue("name", name);
	player.Send("Loaded meower with name: '" + name + "' from database.\r\n");
}

void OnPlayerInput(Player@ player, string rawinput)
{
	string input;
	TrimString(rawinput, input);
	if("makeuuid" == rawinput)
	{
		uuid newuuid;
		newuuid.Generate();
		player.Send("Generated: " + newuuid.ToString() + "\r\n");
		uuid uuid2;
		uuid2 = newuuid;
		player.Send("Copy has uuid " + uuid2.ToString() + "\r\n");
	}
	else if("debugvars" == rawinput)
	{
		game_server.DebugVariables(player);
	}
	else if ("testdb" == rawinput)
	{
		TestDatabase(player);
		player.Send("Ran TestDatabase().\r\n");
	}
	else if("testdbread" == rawinput)
	{
		TestDatabaseRead(player);
	}
	else if ("testobj" == rawinput)
	{
		Log("CALLING TESTOBJ\r\n");
		SuperMeower sm;
		SuperMeower@ hsmeower = @sm;
		Meower@ hmeower = @sm;
		DebugObject(hsmeower);
		DebugObject(hmeower);
	}
	else if("testcmd" == rawinput)
	{
		player.QueueCommand(TestCommand(5, 7), 0, 0);
	}
	else
	{
		switch(player.GetPlayerGameState())
		{
		case PlayerGameState::LOGIN_MENU:
		{
			break;
		}
		case PlayerGameState::ACCOUNT_NAME_ENTRY:
			player.SetName(input);
			player.SetPlayerGameState(PlayerGameState::ACCOUNT_PASSWORD_ENTRY);
			break;
		case PlayerGameState::ACCOUNT_PASSWORD_ENTRY:
		{
			game_server.SendToAll(player.GetName() + " says: " + input + "\r\n");
			string output;
			HashPassword(input, output);
			player.Send("Your password hash is:" + output);
			break;
		}
		default:
			break;
		}
	}
}
