package detalab.DTO;

public class Lobby {
  
  private String ID;
  private int users;
  private String rotation;
  private String creator;
  private String status;

  public Lobby(String ID, int users, String rotation, String creator, String status) {
    this.ID = ID;
    this.users = users;
    this.rotation = rotation;
    this.creator = creator;
    this.status = status;
  }

  public String getLobbyID() {
    return ID;
  }

  public int getLobbyUsers() {
    return users;
  }

  public String getLobbyRotation() {
    return rotation;
  }

  public String getLobbyCreator() {
    return creator;
  }

  public String getLobbyStatus() {
    return status;
  }

}
