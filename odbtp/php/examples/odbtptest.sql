CREATE TABLE dbo.Employees (
	Id int IDENTITY (1, 1) NOT NULL ,
	ExtId numeric (15,0) UNIQUE NOT NULL ,
	LastName varchar (50) NOT NULL ,
	FirstName varchar (50) NOT NULL ,
    Title varchar (256) NOT NULL ,
    Salary money NOT NULL ,
    JobDesc varchar (3000) NULL ,
	Notes ntext NULL ,
	Active bit NOT NULL DEFAULT (1) ,
	DateEntered datetime NOT NULL DEFAULT (getdate()) ,
	DateModified datetime NOT NULL DEFAULT (getdate()) ,
	CONSTRAINT PKCL_Employees_Id PRIMARY KEY  CLUSTERED (
		Id
	)
)
GO

CREATE PROCEDURE AddEmployee
    @ExtId numeric(15,0),
    @LastName varchar(50),
    @FirstName varchar(50),
    @Title varchar(256),
    @Salary money,
    @JobDesc varchar(3000) = 'Job not defined'
AS
    SET NOCOUNT ON

    INSERT INTO Employees( ExtId, LastName, FirstName,
                           Title, Salary, JobDesc )
                   VALUES( @ExtId, @LastName, @FirstName,
                           @Title, @Salary, @JobDesc )

    IF @@ERROR <> 0 RETURN 0
    RETURN @@IDENTITY
GO

CREATE PROCEDURE UpdateEmployee
    @Id int,
    @ExtId numeric(15,0),
    @LastName varchar(50),
    @FirstName varchar(50),
    @Title varchar(256),
    @Salary money,
    @JobDesc varchar(3000)
AS
    SET NOCOUNT ON

    UPDATE Employees SET
      ExtId        = @ExtId,
      LastName     = @LastName,
      FirstName    = @FirstName,
      Title        = @Title,
      Salary       = @Salary,
      JobDesc      = @JobDesc,
      DateModified = getdate()
    WHERE Id = @Id
GO

CREATE PROCEDURE SetEmployeeNotes
    @Id int,
    @Notes ntext
AS
    SET NOCOUNT ON

    UPDATE Employees SET
      Notes = @Notes,
      DateModified = getdate()
    WHERE Id = @Id
GO

CREATE PROCEDURE SetEmployeeActive
    @Id int,
    @Active bit
AS
    SET NOCOUNT ON

    UPDATE Employees SET
      Active = @Active,
      DateModified = getdate()
    WHERE Id = @Id
GO

CREATE PROCEDURE ListEmployees
AS
    SET NOCOUNT ON

    SELECT * FROM Employees ORDER BY LastName, FirstName
GO

CREATE PROCEDURE GetEmployee
    @Id int
AS
    SET NOCOUNT ON

    SELECT * FROM Employees WHERE Id = @Id
GO

