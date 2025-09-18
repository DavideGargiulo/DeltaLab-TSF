package detalab.DTO;

public class User {

  private String email;
  private String username;
  // private String password;
  private String language;

  public User(String email, String username, String language) {
    this.email = email;
    this.username = username;
    this.language = language;
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

}