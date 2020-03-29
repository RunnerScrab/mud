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

class TestPOD
{
	string m_name;
	string m_ability;
	TestPOD(string name, string ability)
	{
		m_name = name;
		m_ability = ability;
	}
};

class Meower : TestInterface, IPersistent
{
	uuid m_uuid;
	string m_name;
	array<TestPOD@> m_testpods;

	void OnLoad(DBTable@ table, DBRow@ row)
	{
		row.GetColValue("uuid", m_uuid);
		row.GetColValue("name", m_name);
		array<DBRow@> resultarr;
		DBTable@ podsubtable = table.GetSubTable("testpodarray");
		if(@podsubtable !is null)
		{
			podsubtable.LoadSubTable(row, resultarr);
			for(int i = 0, len = resultarr.length(); i < len; ++i)
			{
				string name;
				string ability;
				DBRow@ hRow = resultarr[i];
				hRow.GetColValue("name", name);
				hRow.GetColValue("ability", ability);
				m_testpods.insertLast(TestPOD(name, ability));
			}
		}
	}

	void OnDefineSchema(DBTable@ table)
	{
		Log("Calling Meower's DefineSchema()\n");
		table.AddUUIDCol("uuid", DBKEYTYPE_PRIMARY);
		table.AddTextCol("name");
		DBTable@ testpodtable = table.CreateSubTable("testpodarray");
		testpodtable.AddTextCol("name");
		testpodtable.AddTextCol("ability");
	}

	void OnSave(DBTable@ table, DBRow@ row)
	{
		Log("Calling Meower OnSave().\r\n");
		if(@row !is null)
		{
			row.SetColValue("uuid", m_uuid);
			row.SetColValue("name", m_name);
			DBTable@ testpodtable = table.GetSubTable("testpodarray");

			for(int i = 0, len = m_testpods.length();
				i < len; ++i)
				{
					//tptrow.ClearValues();
					TestPOD@ thispod = m_testpods[i];
					DBRow@ tptrow = DBRow(testpodtable);
					tptrow.SetColValue("subtable_index", i);
					tptrow.SetColValue("name", thispod.m_name);
					tptrow.SetColValue("ability", thispod.m_ability);
					row.StoreChildRow(tptrow);
				}

		}
		else
		{
			Log("OnSave was passed a null pointer.\n");
		}
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
		m_testpods.insertLast(TestPOD("pod1", "meow"));
		m_testpods.insertLast(TestPOD("pod2", "MEOW"));
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

	void OnDefineSchema(DBTable@ table)
	{
		Log("Calling SuperMeower's OnDefineSchema()\n");
		table.AddTextCol("superpower");
	}

	SuperMeower()
	{
		Log("SuperMeower constructing.\n");
		TestInterfaceMethod();
		m_superpowername = "meowing";
	}
	void OnSave(DBTable@ table, DBRow@ row)
	{
		Log("Calling SuperMeower OnSave().\r\n");
		row.SetColValue("superpower", m_superpowername);
	}
	void OnLoad(DBTable@ table, DBRow@ row)
	{
		row.GetColValue("superpower", m_superpowername);
	}

};

class MegaMeower : SuperMeower
{
	MegaMeower()
	{

	}
	void OnDefineSchema(DBTable@ table)
	{
		Log("Calling MegaMeower's OnDefineSchema()");
	}

};

enum PlayerGameState {LOGIN_MENU = 0,
		      ACCOUNT_NAME_ENTRY, ACCOUNT_PASSWORD_ENTRY };
class Player : PlayerConnection
{

	Player()
	{
		//super();
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
		player.Send("Account: ");
		//ref@ h = @player;
		/*
		player.Send("Hello!\r\n");

		uuid newuuid;
		newuuid.Generate();
		player.Send("\r\n" + newuuid.ToString() + "\r\n");
		g_meowers.insertLast(SuperMeower());
		player.SetMeower(g_meowers[g_meowers.length() - 1]);
		g_players.insertLast(player);

		game_server.SendToAll("\r\nSomeone has connected. There are " + g_players.length() + " players connected.\r\n");
		*/
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
	testtable.AddUUIDCol("uuid", DBKEYTYPE_PRIMARY);
	testtable.AddTextCol("name");
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
	SuperMeower meower;
	SuperMeower@ hMeower = @meower;
	if(DBSaveObject(@hMeower))
	{
		player.Send("Successfully saved meower to database.\r\n");
	}
	else
	{
		player.Send("Failed to save meower to database.\r\n");
	}
	player.Send("meower guid: " + meower.m_uuid.ToString() + "\r\n");
}

void TestDatabaseRead(Player@ player)
{
	DBTable testtable("testtable");
	testtable.AddUUIDCol("uuid", DBKEYTYPE_PRIMARY);
	testtable.AddTextCol("name");
	DBRow@ testrow = testtable.MakeRow();
	uuid keyuuid;
	keyuuid.FromString("0e6e8006-698a-46ec-8aa4-211fa6a1892c");
	testrow.SetColValue("uuid", keyuuid);
	testrow.LoadFromDB();
	string name;
	testrow.GetColValue("name", name);
	player.Send("Loaded meower with name: '" + name + "' from database.\r\n");
	SuperMeower meower;

	if(keyuuid.FromString("0f778a4b-71fb-4030-9da6-6a322a102033"))
	{
		try
		{
			if(DBLoadObject(@meower, keyuuid))
			{
				player.Send("Successfully loaded meower with key 'testkey'.\r\n");
				player.Send("Meower power: " + meower.m_superpowername + "\r\n");
				for(int i = 0, len = meower.m_testpods.length(); i < len; ++i)
				{
					player.Send("Meower testpod name: " +meower.m_testpods[i].m_name +
							    " ability: " + meower.m_testpods[i].m_ability + "\r\n");
				}
			}
			else
			{
				player.Send("Failed to load SuperMeower with key 'testkey'.\r\n");
			}
		}
		catch
		{
			player.Send("`red`" + getExceptionInfo() + "`default`\r\n");
		}
	}

	const DBTable@ hTable = DBGetClassTable(meower);
	if(hTable !is null)
	{
		player.Send("Acquired table name: " + hTable.GetName() + "\r\n");
	}
}

void OnPlayerInput(Player@ player, string rawinput)
{
	string input;
	TrimString(rawinput, input);
	//This command handling is for testing only
	//Real Commands should be put in a map and assisted with an argument parsing/tokenizing function
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
	else if ("testcolor" == rawinput)
	{
		player.Send("`red`Testing color!`default`\r\n");
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
