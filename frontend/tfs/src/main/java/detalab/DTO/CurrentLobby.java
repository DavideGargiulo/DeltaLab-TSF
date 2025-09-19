package detalab.DTO;

import java.util.ArrayList;

import detalab.DTO.User;
import detalab.DTO.Lobby;

public class CurrentLobby extends Lobby {

  ArrayList<User> playingUsers;
  ArrayList<User> spectators;

  //Instance
  private static CurrentLobby instance;

  //Constructor
  private CurrentLobby(Lobby lobby, ArrayList<User> playingUsers, ArrayList<User> spectators) {
    super(lobby.getLobbyID(), lobby.getLobbyUsers(), lobby.getLobbyRotation(), lobby.getLobbyCreator(), lobby.getLobbyStatus());
    this.playingUsers = playingUsers;
    this.spectators = spectators;
  }

  public static CurrentLobby getInstance(Lobby lobby, ArrayList<User> playingUsers, ArrayList<User> spectators) {
    if(instance == null)
      instance = new CurrentLobby(lobby, playingUsers, spectators);
    return instance;
  }

  public static CurrentLobby getInstance() {
    return instance;
  }

  public static void cleanLobby() {
    instance = null;
  }

}