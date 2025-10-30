package detalab.DTO;

import java.util.ArrayList;

public class CurrentLobby extends Lobby {

  ArrayList<User> playingUsers;
  ArrayList<User> spectators;

  // Instance
  private static CurrentLobby instance;

  // Constructor
  private CurrentLobby(Lobby lobby, ArrayList<User> playingUsers, ArrayList<User> spectators) {
    super(lobby.getLobbyID(), lobby.getLobbyUsers(), lobby.getLobbyRotation(), lobby.getLobbyCreator(),
        lobby.getLobbyStatus());
    this.playingUsers = playingUsers;
    this.spectators = spectators;
  }

  public static CurrentLobby getInstance(Lobby lobby, ArrayList<User> playingUsers, ArrayList<User> spectators) {
    if (instance == null)
      instance = new CurrentLobby(lobby, playingUsers, spectators);
    return instance;
  }

  public static CurrentLobby getInstance() {
    return instance;
  }

  public static void cleanLobby() {
    instance = null;
  }

  public ArrayList<User> getPlayers() {
    return playingUsers;
  }

  public ArrayList<User> getSpectators() {
    return spectators;
  }

  public void setPlayers(ArrayList<User> playingUsers) {
    this.playingUsers = playingUsers;
  }

  public void setSpectators(ArrayList<User> spectators) {
    this.spectators = spectators;
  }

  @Override
  public String toString() {
    return "CurrentLobby{" +
        "ID='" + getLobbyID() + '\'' +
        ", users=" + getLobbyUsers() +
        ", rotation='" + getLobbyRotation() + '\'' +
        ", creator='" + getLobbyCreator() + '\'' +
        ", status='" + getLobbyStatus() + '\'' +
        ", playingUsers=" + playingUsers +
        ", spectators=" + spectators +
        '}';
  }

}