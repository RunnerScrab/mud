class TestCommand : IAction
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
		game_server.SendToAll("`blink``bgblue`Ran TestCommand. Result: " + sum + "`default`\r\n");
		return 0;
	}
};

interface TestInterface
{
	void TestInterfaceMethod();
};

class IPlayerInputMode
{
	int OnInputReceived(string &in input)
	{
		return 0;
	}

	void SetPlayerConnection(PlayerConnection@ conn)
	{

	}
};

class PlayerEditInputMode : IPlayerInputMode
{
	LineEditor@ leditor;
        Player@ player;

	PlayerEditInputMode(Player@ p, EditableText@ target)
	{
		@player = p;
		@leditor = LineEditor();
		leditor.SetEditTarget(target);
		leditor.SetPlayerConnection(player.m_connection.get());
	}

	int OnInputReceived(string &in input)
	{
		input += "\n";
		return leditor.ProcessInput(input);
	}

	void SetPlayerConnection(PlayerConnection@ conn)
	{
		leditor.SetPlayerConnection(conn);
	}

};

int TestFunction2(Player@ player, PlayerConnection@ conn)
{
	conn.Send("Ran TestFunction2()!\r\n");
	return 0;
}

funcdef int CmdFunc(Player@ player, PlayerConnection@ conn);
class PlayerDefaultInputMode : IPlayerInputMode
{
	weakref<PlayerConnection> pconn;
	Player@ player;
	dictionary commanddict;

	PlayerDefaultInputMode(Player@ p)
	{
		CmdFunc@ testfunc = function(player, conn)
			 {
				 conn.Send("Test dictionary anon function run!\n");
				 return 0;
			 };
		commanddict = {
			{'testdict', testfunc},
			{'testdict2', TestFunction2}
		};
		@player = p;
		pconn = p.m_connection;
	}

	int OnInputReceived(string &in input)
	{
		PlayerConnection@ conn = pconn.get();
		if(conn !is null)
		{
			conn.Send("You input: " + input + "\r\n");
			if(input == "quit")
			{
				conn.Disconnect();
			}
			else if("kill" == input)
			{
				conn.Send("Killing server.\n");
				game_server.Kill();
			}
			else if("testfunc" == input)
				TestFunction(conn);
			else if ("testdb" == input)
			{
				TestDatabase(player);
				conn.Send("Ran TestDatabase().\r\n");
			}
			else if("testdbread" == input)
			{
				TestDatabaseRead(player);
			}
			else if("testmp" == input)
			{
				MPInt num = "-1234567891011121314151617";
				conn.Send("MPInt test: '" + num.toString() + "'\r\n");
				num = 67890;

				MPInt num2 = num;
				num2 = num + int(3.5);
				conn.Send("MPInt num2 initialized to: " + num2.toString() + "\r\n");
				num2 *= 3;
				player.Send("MPInt test two: '" + num2.toString() + "'\r\n");
				MPFloat fpoint = 1.75;
				MPFloat fpoint2 = 1.0;
				fpoint = fpoint - fpoint2;
				fpoint = sin(fpoint);
				conn.Send("MPFloat test result: " + fpoint.toString(9) + "\r\n");
			}
			else if("tc" == input)
			{
				player.m_char.QueueAction(TestCommand(5, 7), 0, 0);
				conn.Send("Command received.\r\n");
			}
			else if("tdc" == input)
			{
				player.m_char.QueueAction(TestCommand(1, 9), 6, 0);
				conn.Send("Command received.\r\n");
			}
			else
			{
				if(commanddict.exists(input))
				{
					CmdFunc@ func;
					if(commanddict.get(input, @func))
					{
						func(player, conn);
					}
				}
			}
		}
		return 0;

	}

	void SetPlayerConnection(PlayerConnection@ c)
	{

	}
};

TestCommand tc(1, 2);
Player@ hPlayer = null;
array<Player@> g_players;
array<Meower@> g_meowers;
void GameTick()
{
	return;
	game_server.SendToAll("`red`Hello!`default`");
	game_server.QueueGlobalAction(tc, 4);
	game_server.QueueGlobalAction(tc, 2);
	if(hPlayer !is null)
	{
		hPlayer.Send("Ticking for player\r\n");
	}
	else
	{
		Log("There are no players connected.\r\n");
	}

}

void OnPlayerConnect(PlayerConnection@ conn)
{
	try
	{
		Player@ player = Player(conn);
		g_players.insertLast(player);
		if(g_players.length() >= 2)
		{
			player.Send("Attaching an additional observer.\n");
		}
		uuid newuuid;
		newuuid.Generate();
		player.Send("\r\n" + newuuid.ToString() + "\r\n");
		g_meowers.insertLast(SuperMeower());
		player.SetMeower(g_meowers[g_meowers.length() - 1]);

		game_server.SendToAll("\r\nSomeone has connected. There are " + g_players.length() + " players connected.\r\n");

	}
	catch
	{
		Log("Exception thrown!\r\n" + getExceptionInfo() + "\r\n");
	}
}

void OnPlayerDisconnect(PlayerConnection@ player)
{

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
	meower.m_testpods.insertLast(TestPOD("pod1", "meow"));
	meower.m_testpods.insertLast(TestPOD("pod2", "MEOW"));

	SuperMeower@ hMeower = @meower;
	if(DBSaveObject(@hMeower))
	{
		Log("Successfully saved meower to database.\r\n");
		player.Send("Successfully saved meower to database.\r\n");
		player.Send("`#00ff00`Successfully saved meower to database.`default`\r\n");
	}
	else
	{
		Log("Failed to save meower to database.\r\n");
		player.Send("Failed to save meower to database.\r\n");
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

	keyuuid.FromString("4494a92a-232e-414f-a778-e30bc01e756c");

	testrow.SetColValue("uuid", keyuuid);
	testrow.LoadFromDB();
	string name;
	testrow.GetColValue("name", name);

	SuperMeower meower;

	if(keyuuid.FromString("2a00e8ba-2104-41da-80e5-5e61c84abf57"))
	{
		try
		{
			if(DBLoadObject(@meower, keyuuid))
			{
				player.Send("Successfully loaded meower with name " + meower.m_name + ".\r\n");
				player.Send("Meower power: " + meower.m_superpowername + "\r\n");
				player.Send("Meower xcoord: " + meower.m_xval.toString() + "\r\n");
				player.Send("Meower ycoord: " + meower.m_yval.toString() + "\r\n");

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

	//const DBTable@ hTable = DBGetClassTable(meower);
	const DBTable@ hTable = DBGetClassTableByName("SuperMeower");
	if(hTable !is null)
	{
		player.Send("Acquired table name: " + hTable.GetName() + "\r\n");
	}
}

void OnPlayerInput(Player@ player, string rawinput)
{
	string input = TrimString(rawinput);

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
	else if ("testcolor" == rawinput)
	{
		player.Send("`#00ff00`Testing color!`default`\r\n");
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
	else if("tc" == rawinput)
	{
		player.m_char.QueueAction(TestCommand(5, 7), 0, 0);
		player.Send("Command received.\r\n");
	}
	else if("tdc" == rawinput)
	{
		player.m_char.QueueAction(TestCommand(1, 9), 6, 0);
		player.Send("Command received.\r\n");
	}
	else
	{
		/*
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
			string output = HashPassword(input);
			player.Send("Your password hash is:" + output);
			break;
		}
		default:
			break;
		}
		*/
	}
}
