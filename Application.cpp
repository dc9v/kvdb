#include "Application.h"

void handler(int sig)
{
	void *array[10];
	size_t size;


	size = backtrace(array, 10);


	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

int main(int argc, char *argv[])
{

	if (argc != ARGS_COUNT)
	{
		cout << "Configuration (i.e., *.conf) file File Required" << endl;
		return FAILURE;
	}


	Application *app = new Application(argv[1]);

	app->run();

	delete (app);

	return SUCCESS;
}
Application::Application(char *infile)
{
	int i;
	par = new Params();
	srand(time(NULL));
	par->setparams(infile);
	log = new Log(par);
	en = new EmulNet(par);
	en1 = new EmulNet(par);
	mp1 = (MP1Node **)malloc(par->EN_GPSZ * sizeof(MP1Node *));
	mp2 = (MP2Node **)malloc(par->EN_GPSZ * sizeof(MP2Node *));

	for (i = 0; i < par->EN_GPSZ; i++)
	{
		Member *memberNode = new Member;
		memberNode->inited = false;
		Address *addressOfMemberNode = new Address();
		Address joinaddr;
		joinaddr = getjoinaddr();
		addressOfMemberNode = (Address *)en->ENinit(addressOfMemberNode, par->PORTNUM);
		mp1[i] = new MP1Node(memberNode, par, en, log, addressOfMemberNode);
		mp2[i] = new MP2Node(memberNode, par, en1, log, addressOfMemberNode);
		log->LOG(&(mp1[i]->getMemberNode()->addr), "APP");
		log->LOG(&(mp2[i]->getMemberNode()->addr), "APP MP2");
		delete addressOfMemberNode;
	}
}
Application::~Application()
{
	delete log;
	delete en;
	delete en1;
	for (int i = 0; i < par->EN_GPSZ; i++)
	{
		delete mp1[i];
		delete mp2[i];
	}
	free(mp1);
	free(mp2);
	delete par;
}

int Application::run()
{
	int i;
	int timeWhenAllNodesHaveJoined = 0;

	bool allNodesJoined = false;
	srand(time(NULL));


	for (par->globaltime = 0; par->globaltime < TOTAL_RUNNING_TIME; ++par->globaltime)
	{

		mp1Run();


		if (par->allNodesJoined == nodeCount && !allNodesJoined)
		{
			timeWhenAllNodesHaveJoined = par->getcurrtime();
			allNodesJoined = true;
		}
		if (par->getcurrtime() > timeWhenAllNodesHaveJoined + 50)
		{

			mp2Run();
		}


	}


	en->ENcleanup();
	en1->ENcleanup();

	for (i = 0; i <= par->EN_GPSZ - 1; i++)
	{
		mp1[i]->finishUpThisNode();
	}

	return SUCCESS;
}

void Application::mp1Run()
{
	int i;


	for (i = 0; i <= par->EN_GPSZ - 1; i++)
	{


		if (par->getcurrtime() > (int)(par->STEP_RATE * i) && !(mp1[i]->getMemberNode()->bFailed))
		{

			mp1[i]->recvLoop();
		}
	}


	for (i = par->EN_GPSZ - 1; i >= 0; i--)
	{


		if (par->getcurrtime() == (int)(par->STEP_RATE * i))
		{

			mp1[i]->nodeStart(JOINADDR, par->PORTNUM);
			cout << i << "-th introduced node is assigned with the address: " << mp1[i]->getMemberNode()->addr.getAddress() << endl;
			nodeCount += i;
		}


		else if (par->getcurrtime() > (int)(par->STEP_RATE * i) && !(mp1[i]->getMemberNode()->bFailed))
		{

			mp1[i]->nodeLoop();
#ifdef DEBUGLOG
			if ((i == 0) && (par->globaltime % 500 == 0))
			{
				log->LOG(&mp1[i]->getMemberNode()->addr, "@@time=%d", par->getcurrtime());
			}
#endif
		}
	}
}

void Application::mp2Run()
{
	int i;


	for (i = 0; i <= par->EN_GPSZ - 1; i++)
	{


		if (par->getcurrtime() > (int)(par->STEP_RATE * i) && !mp2[i]->getMemberNode()->bFailed)
		{
			if (mp2[i]->getMemberNode()->inited && mp2[i]->getMemberNode()->inGroup)
			{

				mp2[i]->updateRing();
			}

			mp2[i]->recvLoop();
		}
	}

	for (i = par->EN_GPSZ - 1; i >= 0; i--)
	{
		if (par->getcurrtime() > (int)(par->STEP_RATE * i) && !mp2[i]->getMemberNode()->bFailed)
		{
			mp2[i]->checkMessages();
		}
	}

	if (par->getcurrtime() == INSERT_TIME)
	{
		insertTestKVPairs();
	}

	if (par->getcurrtime() >= TEST_TIME)
	{
		if (par->getcurrtime() == TEST_TIME && CREATE_TEST == par->CRUDTEST)
		{
			cout << endl
					 << "Doing create test at time: " << par->getcurrtime() << endl;
		}

		else if (par->getcurrtime() == TEST_TIME && DELETE_TEST == par->CRUDTEST)
		{
			deleteTest();
		}

		else if (par->getcurrtime() >= TEST_TIME && READ_TEST == par->CRUDTEST)
		{
			readTest();
		}

		else if (par->getcurrtime() >= TEST_TIME && UPDATE_TEST == par->CRUDTEST)
		{
			updateTest();
		}

	}
}
void Application::fail()
{
	int i, removed;


	if (par->DROP_MSG && par->getcurrtime() == 50)
	{
		par->dropmsg = 1;
	}

	if (par->SINGLE_FAILURE && par->getcurrtime() == 100)
	{
		removed = (rand() % par->EN_GPSZ);
#ifdef DEBUGLOG
		log->LOG(&mp1[removed]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
#endif
		mp1[removed]->getMemberNode()->bFailed = true;
	}
	else if (par->getcurrtime() == 100)
	{
		removed = rand() % par->EN_GPSZ / 2;
		for (i = removed; i < removed + par->EN_GPSZ / 2; i++)
		{
#ifdef DEBUGLOG
			log->LOG(&mp1[i]->getMemberNode()->addr, "Node failed at time = %d", par->getcurrtime());
#endif
			mp1[i]->getMemberNode()->bFailed = true;
		}
	}

	if (par->DROP_MSG && par->getcurrtime() == 300)
	{
		par->dropmsg = 0;
	}
}

Address Application::getjoinaddr(void)
{

	Address joinaddr;
	joinaddr.init();
	*(int *)(&(joinaddr.addr)) = 1;
	*(short *)(&(joinaddr.addr[4])) = 0;

	return joinaddr;
}

int Application::findARandomNodeThatIsAlive()
{
	int number;
	do
	{
		number = (rand() % par->EN_GPSZ);
	} while (mp2[number]->getMemberNode()->bFailed);
	return number;
}

void Application::initTestKVPairs()
{
	srand(time(NULL));
	int i;
	string key;
	key.clear();
	testKVPairs.clear();
	int alphanumLen = sizeof(alphanum) - 1;
	while (testKVPairs.size() != NUMBER_OF_INSERTS)
	{
		for (i = 0; i < KEY_LENGTH; i++)
		{
			key.push_back(alphanum[rand() % alphanumLen]);
		}
		string value = "value" + to_string(rand() % NUMBER_OF_INSERTS);
		testKVPairs[key] = value;
		key.clear();
	}
}

void Application::insertTestKVPairs()
{
	int number = 0;


	initTestKVPairs();

	for (map<string, string>::iterator it = testKVPairs.begin(); it != testKVPairs.end(); ++it)
	{

		number = findARandomNodeThatIsAlive();


		log->LOG(&mp2[number]->getMemberNode()->addr, "CREATE OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), it->second.c_str(), par->getcurrtime());
		mp2[number]->clientCreate(it->first, it->second);
	}

	cout << endl
			 << "Sent " << testKVPairs.size() << " create messages to the ring" << endl;
}

void Application::deleteTest()
{
	int number;
	cout << endl
			 << "Deleting " << testKVPairs.size() / 2 << " valid keys.... ... .. . ." << endl;
	map<string, string>::iterator it = testKVPairs.begin();
	for (int i = 0; i < testKVPairs.size() / 2; i++)
	{
		it++;


		number = findARandomNodeThatIsAlive();


		log->LOG(&mp2[number]->getMemberNode()->addr, "DELETE OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), it->second.c_str(), par->getcurrtime());
		mp2[number]->clientDelete(it->first);
	}

	cout << endl
			 << "Deleting an invalid key.... ... .. . ." << endl;
	string invalidKey = "invalidKey";

	number = findARandomNodeThatIsAlive();


	log->LOG(&mp2[number]->getMemberNode()->addr, "DELETE OPERATION KEY: %s at time: %d", invalidKey.c_str(), par->getcurrtime());
	mp2[number]->clientDelete(invalidKey);
}

void Application::readTest()
{



	map<string, string>::iterator it = testKVPairs.begin();
	int number;
	vector<Node> replicas;
	int replicaIdToFail = TERTIARY;
	int nodeToFail;
	bool failedOneNode = false;

	if (par->getcurrtime() == TEST_TIME)
	{

		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Reading a valid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "READ OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), it->second.c_str(), par->getcurrtime());
		mp2[number]->clientRead(it->first);
	}


	if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME))
	{

		number = findARandomNodeThatIsAlive();


		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);

		if (replicas.size() < (RF - 1))
		{
			cout << endl
					 << "Could not find at least quorum replicas for this key. Exiting!!! size of replicas vector: " << replicas.size() << endl;
			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not find at least quorum replicas for this key. Exiting!!! size of replicas vector: %d", replicas.size());
			exit(1);
		}


		for (int i = 0; i < par->EN_GPSZ; i++)
		{
			if (mp2[i]->getMemberNode()->addr.getAddress() == replicas.at(replicaIdToFail).getAddress()->getAddress())
			{
				if (!mp2[i]->getMemberNode()->bFailed)
				{
					nodeToFail = i;
					failedOneNode = true;
					break;
				}
				else
				{

					if (replicaIdToFail > 0)
					{
						replicaIdToFail--;
					}
					else
					{
						failedOneNode = false;
					}
				}
			}
		}
		if (failedOneNode)
		{
			log->LOG(&mp2[nodeToFail]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
			mp2[nodeToFail]->getMemberNode()->bFailed = true;
			mp1[nodeToFail]->getMemberNode()->bFailed = true;
			cout << endl
					 << "Failed a replica node" << endl;
		}
		else
		{

			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail a node");
			cout << "Could not fail a node. Exiting!!!";
			exit(1);
		}

		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Reading a valid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "READ OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), it->second.c_str(), par->getcurrtime());
		mp2[number]->clientRead(it->first);

		failedOneNode = false;
	}



	if (par->getcurrtime() >= (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME))
	{
		vector<int> nodesToFail;
		nodesToFail.clear();
		int count = 0;

		if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME))
		{

			number = findARandomNodeThatIsAlive();


			replicas.clear();
			replicas = mp2[number]->findNodes(it->first);



			if (replicas.size() > 2)
			{
				replicaIdToFail = TERTIARY;
				while (count != 2)
				{
					int i = 0;
					while (i != par->EN_GPSZ)
					{
						if (mp2[i]->getMemberNode()->addr.getAddress() == replicas.at(replicaIdToFail).getAddress()->getAddress())
						{
							if (!mp2[i]->getMemberNode()->bFailed)
							{
								nodesToFail.emplace_back(i);
								replicaIdToFail--;
								count++;
								break;
							}
							else
							{

								if (replicaIdToFail > 0)
								{
									replicaIdToFail--;
								}
							}
						}
						i++;
					}
				}
			}
			else
			{

				cout << endl
						 << "Not enough replicas to fail two nodes. Number of replicas of this key: " << replicas.size() << ". Exiting test case !! " << endl;
				exit(1);
			}
			if (count == 2)
			{
				for (int i = 0; i < nodesToFail.size(); i++)
				{

					log->LOG(&mp2[nodesToFail.at(i)]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
					mp2[nodesToFail.at(i)]->getMemberNode()->bFailed = true;
					mp1[nodesToFail.at(i)]->getMemberNode()->bFailed = true;
					cout << endl
							 << "Failed a replica node" << endl;
				}
			}
			else
			{

				log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail two nodes");

				cout << "Could not fail two nodes. Exiting!!!";
				exit(1);
			}

			number = findARandomNodeThatIsAlive();


			cout << endl
					 << "Reading a valid key.... ... .. . ." << endl;
			log->LOG(&mp2[number]->getMemberNode()->addr, "READ OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), it->second.c_str(), par->getcurrtime());

			mp2[number]->clientRead(it->first);
		}


		if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME + STABILIZE_TIME))
		{
			number = findARandomNodeThatIsAlive();

			cout << endl
					 << "Reading a valid key.... ... .. . ." << endl;
			log->LOG(&mp2[number]->getMemberNode()->addr, "READ OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), it->second.c_str(), par->getcurrtime());

			mp2[number]->clientRead(it->first);
		}
	}


	if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME + STABILIZE_TIME + LAST_FAIL_TIME))
	{

		number = findARandomNodeThatIsAlive();


		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);
		for (int i = 0; i < par->EN_GPSZ; i++)
		{
			if (!mp2[i]->getMemberNode()->bFailed)
			{
				if (mp2[i]->getMemberNode()->addr.getAddress() != replicas.at(PRIMARY).getAddress()->getAddress() &&
						mp2[i]->getMemberNode()->addr.getAddress() != replicas.at(SECONDARY).getAddress()->getAddress() &&
						mp2[i]->getMemberNode()->addr.getAddress() != replicas.at(TERTIARY).getAddress()->getAddress())
				{

					log->LOG(&mp2[i]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
					mp2[i]->getMemberNode()->bFailed = true;
					mp1[i]->getMemberNode()->bFailed = true;
					failedOneNode = true;
					cout << endl
							 << "Failed a non-replica node" << endl;
					break;
				}
			}
		}
		if (!failedOneNode)
		{

			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail a node(non-replica)");
			cout << "Could not fail a node(non-replica). Exiting!!!";
			exit(1);
		}

		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Reading a valid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "READ OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), it->second.c_str(), par->getcurrtime());

		mp2[number]->clientRead(it->first);
	}


	if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME + STABILIZE_TIME + LAST_FAIL_TIME))
	{
		string invalidKey = "invalidKey";


		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Reading an invalid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "READ OPERATION KEY: %s at time: %d", invalidKey.c_str(), par->getcurrtime());

		mp2[number]->clientRead(invalidKey);
	}

}

void Application::updateTest()
{


	map<string, string>::iterator it = testKVPairs.begin();
	it++;
	string newValue = "newValue";
	int number;
	vector<Node> replicas;
	int replicaIdToFail = TERTIARY;
	int nodeToFail;
	bool failedOneNode = false;

	if (par->getcurrtime() == TEST_TIME)
	{

		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Updating a valid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "UPDATE OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), newValue.c_str(), par->getcurrtime());
		mp2[number]->clientUpdate(it->first, newValue);
	}


	if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME))
	{

		number = findARandomNodeThatIsAlive();


		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);

		if (replicas.size() < RF - 1)
		{
			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not find at least quorum replicas for this key. Exiting!!! size of replicas vector: %d", replicas.size());
			cout << endl
					 << "Could not find at least quorum replicas for this key. Exiting!!! size of replicas vector: " << replicas.size() << endl;
			exit(1);
		}


		for (int i = 0; i < par->EN_GPSZ; i++)
		{
			if (mp2[i]->getMemberNode()->addr.getAddress() == replicas.at(replicaIdToFail).getAddress()->getAddress())
			{
				if (!mp2[i]->getMemberNode()->bFailed)
				{
					nodeToFail = i;
					failedOneNode = true;
					break;
				}
				else
				{

					if (replicaIdToFail > 0)
					{
						replicaIdToFail--;
					}
					else
					{
						failedOneNode = false;
					}
				}
			}
		}
		if (failedOneNode)
		{
			log->LOG(&mp2[nodeToFail]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
			mp2[nodeToFail]->getMemberNode()->bFailed = true;
			mp1[nodeToFail]->getMemberNode()->bFailed = true;
			cout << endl
					 << "Failed a replica node" << endl;
		}
		else
		{

			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail a node");
			cout << "Could not fail a node. Exiting!!!";
			exit(1);
		}

		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Updating a valid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "UPDATE OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), newValue.c_str(), par->getcurrtime());
		mp2[number]->clientUpdate(it->first, newValue);

		failedOneNode = false;
	}


	if (par->getcurrtime() >= (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME))
	{

		vector<int> nodesToFail;
		nodesToFail.clear();
		int count = 0;

		if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME))
		{

			number = findARandomNodeThatIsAlive();


			replicas.clear();
			replicas = mp2[number]->findNodes(it->first);


			if (replicas.size() > 2)
			{
				replicaIdToFail = TERTIARY;
				while (count != 2)
				{
					int i = 0;
					while (i != par->EN_GPSZ)
					{
						if (mp2[i]->getMemberNode()->addr.getAddress() == replicas.at(replicaIdToFail).getAddress()->getAddress())
						{
							if (!mp2[i]->getMemberNode()->bFailed)
							{
								nodesToFail.emplace_back(i);
								replicaIdToFail--;
								count++;
								break;
							}
							else
							{

								if (replicaIdToFail > 0)
								{
									replicaIdToFail--;
								}
							}
						}
						i++;
					}
				}
			}
			else
			{

				cout << endl
						 << "Not enough replicas to fail two nodes. Exiting test case !! " << endl;
			}
			if (count == 2)
			{
				for (int i = 0; i < nodesToFail.size(); i++)
				{

					log->LOG(&mp2[nodesToFail.at(i)]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
					mp2[nodesToFail.at(i)]->getMemberNode()->bFailed = true;
					mp1[nodesToFail.at(i)]->getMemberNode()->bFailed = true;
					cout << endl
							 << "Failed a replica node" << endl;
				}
			}
			else
			{

				log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail two nodes");
				cout << "Could not fail two nodes. Exiting!!!";
				exit(1);
			}

			number = findARandomNodeThatIsAlive();


			cout << endl
					 << "Updating a valid key.... ... .. . ." << endl;
			log->LOG(&mp2[number]->getMemberNode()->addr, "UPDATE OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), newValue.c_str(), par->getcurrtime());

			mp2[number]->clientUpdate(it->first, newValue);
		}


		if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME + STABILIZE_TIME))
		{
			number = findARandomNodeThatIsAlive();

			cout << endl
					 << "Updating a valid key.... ... .. . ." << endl;
			log->LOG(&mp2[number]->getMemberNode()->addr, "UPDATE OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), newValue.c_str(), par->getcurrtime());

			mp2[number]->clientUpdate(it->first, newValue);
		}
	}


	if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME + STABILIZE_TIME + LAST_FAIL_TIME))
	{

		number = findARandomNodeThatIsAlive();


		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);
		for (int i = 0; i < par->EN_GPSZ; i++)
		{
			if (!mp2[i]->getMemberNode()->bFailed)
			{
				if (mp2[i]->getMemberNode()->addr.getAddress() != replicas.at(PRIMARY).getAddress()->getAddress() &&
						mp2[i]->getMemberNode()->addr.getAddress() != replicas.at(SECONDARY).getAddress()->getAddress() &&
						mp2[i]->getMemberNode()->addr.getAddress() != replicas.at(TERTIARY).getAddress()->getAddress())
				{

					log->LOG(&mp2[i]->getMemberNode()->addr, "Node failed at time=%d", par->getcurrtime());
					mp2[i]->getMemberNode()->bFailed = true;
					mp1[i]->getMemberNode()->bFailed = true;
					failedOneNode = true;
					cout << endl
							 << "Failed a non-replica node" << endl;
					break;
				}
			}
		}

		if (!failedOneNode)
		{

			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail a node(non-replica)");
			cout << "Could not fail a node(non-replica). Exiting!!!";
			exit(1);
		}

		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Updating a valid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "UPDATE OPERATION KEY: %s VALUE: %s at time: %d", it->first.c_str(), newValue.c_str(), par->getcurrtime());

		mp2[number]->clientUpdate(it->first, newValue);
	}


	if (par->getcurrtime() == (TEST_TIME + FIRST_FAIL_TIME + STABILIZE_TIME + STABILIZE_TIME + LAST_FAIL_TIME))
	{
		string invalidKey = "invalidKey";
		string invalidValue = "invalidValue";


		number = findARandomNodeThatIsAlive();


		cout << endl
				 << "Updating a valid key.... ... .. . ." << endl;
		log->LOG(&mp2[number]->getMemberNode()->addr, "UPDATE OPERATION KEY: %s VALUE: %s at time: %d", invalidKey.c_str(), invalidValue.c_str(), par->getcurrtime());

		mp2[number]->clientUpdate(invalidKey, invalidValue);
	}

}
