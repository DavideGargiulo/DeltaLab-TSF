package detalab.DTO;

public class User {

  private int id;
  private String email;
  private String username;
  private String language;
  private String status;

  public User(int id, String email, String username, String language) {
    this.id = id;
    this.email = email;
    this.username = username;
    this.language = language;
  }

  public User(int id, String username, String status) {
    this.id = id;
    this.username = username;
    this.status = status;
  }

  public int getId() {
    return id;
  }

  public String getEmail() {
    return email;
  }

  public String getUsername() {
    return username;
  }

  public String getLanguage() {
    return language;
  }

  public String getStatus() {
    return status;
  }

}