module detalab {
    requires javafx.controls;
    requires javafx.fxml;
    requires org.json;
    requires javafx.graphics;

    opens detalab to javafx.fxml;
    opens detalab.DTO to javafx.base;
    opens detalab.Controller to javafx.fxml;
    exports detalab;
}
