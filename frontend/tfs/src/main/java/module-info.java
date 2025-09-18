module detalab {
    requires javafx.controls;
    requires javafx.fxml;
    requires org.json;

    opens detalab to javafx.fxml;
    opens detalab.DTO to javafx.base;
    exports detalab;
}
