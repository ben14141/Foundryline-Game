#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Production/FoundrylineFactorySimulation.h"
#include "JsonObjectConverter.h"

namespace PlayfulEmployeeNameTest {
bool Example(const FString& N){return N==TEXT("Rusty Sparks")||N==TEXT("Skip Lunch")||N==TEXT("Chip Grinder")||N==TEXT("Doug Dimmweld");}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayfulEmployeeNames,"Foundryline.Employees.PlayfulNamesStableAndMixed",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayfulEmployeeNames::RunTest(const FString&){
 using namespace PlayfulEmployeeNameTest;FFoundrylineFactorySimulation M;M.State.CompanyId=FGuid(11,22,33,44);M.State.PlantId=FGuid(55,66,77,88);bool SawPlayful=false,SawOrdinary=false;int CompanyDifferences=0;
 auto Other=M;Other.State.CompanyId=FGuid(99,22,33,44);
 for(int Id=2000;Id<3024;++Id){const FString Name=M.NewEmployeeName(Id,true);TestEqual(TEXT("Name is stable for a company-owned employee identity"),M.NewEmployeeName(Id,true),Name);SawPlayful|=Example(Name);SawOrdinary|=Name.StartsWith(TEXT("Avery "))||Name.StartsWith(TEXT("Jordan "))||Name.StartsWith(TEXT("Taylor "));CompanyDifferences+=Other.NewEmployeeName(Id,true)!=Name;TestTrue(TEXT("Generated names remain readable and bounded"),Name.Len()>3&&Name.Len()<=48);}
 TestTrue(TEXT("Occasional workshop names appear"),SawPlayful);TestTrue(TEXT("Ordinary names remain in the hiring pool"),SawOrdinary);TestTrue(TEXT("Company identity participates in new-hire name selection"),CompanyDifferences>0);
 const int Id=2200;const FString Used=M.NewEmployeeName(Id,true);FFoundrylineStandbyDriver Existing;Existing.Id=Id+1;Existing.DisplayName=Used;M.State.UnassignedEmployees.Add(Existing);const FString Alternative=M.NewEmployeeName(Id,true);TestTrue(TEXT("Existing roster names are not duplicated"),Alternative!=Used);TestEqual(TEXT("Collision fallback is deterministic"),M.NewEmployeeName(Id,true),Alternative);
 TestFalse(TEXT("Legacy missing-name migration uses the ordinary pool"),Example(M.NewEmployeeName(2050,false)));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayfulEmployeeNamePersistence,"Foundryline.Employees.PlayfulNamesPersistWithoutRenamingStaff",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlayfulEmployeeNamePersistence::RunTest(const FString&){
 using namespace PlayfulEmployeeNameTest;FFoundrylineFactorySimulation M;M.SeedCutPartsShop();const auto OldOperators=M.State.Operators;const auto OldDrivers=M.State.Drivers;FString Error;int Candidate=0;
 for(int Id=M.State.NextId;Id<M.State.NextId+2048;++Id)if(Example(M.NewEmployeeName(Id,true))){Candidate=Id;break;}
 if(!TestTrue(TEXT("A playful identity can be selected for a new hire"),Candidate>0))return false;
 M.State.NextId=Candidate;const FString Expected=M.NewEmployeeName(Candidate,true);
 if(!TestTrue(TEXT("Hire through the normal authoritative simulation API"),M.HireEmployee(EFoundrylineEmployeeRole::Unassigned,FGuid(101,202,303,404),Error))){AddError(Error);return false;}
 const auto* Hired=M.State.UnassignedEmployees.FindByPredicate([&](const auto& E){return E.Id==Candidate;});TestTrue(TEXT("The selected playful name is stored on the employee"),Hired&&Hired->DisplayName==Expected);
 for(int I=0;I<OldOperators.Num();++I)TestEqual(TEXT("Existing operators retain their saved names"),M.State.Operators[I].DisplayName,OldOperators[I].DisplayName);for(int I=0;I<OldDrivers.Num();++I)TestEqual(TEXT("Existing drivers retain their saved names"),M.State.Drivers[I].DisplayName,OldDrivers[I].DisplayName);
 FString Json;TestTrue(TEXT("Serialize named roster"),FJsonObjectConverter::UStructToJsonObjectString(M.State,Json));FFoundrylineFactoryState Loaded;TestTrue(TEXT("Restore named roster"),FJsonObjectConverter::JsonObjectStringToUStruct(Json,&Loaded));const auto* Saved=Loaded.UnassignedEmployees.FindByPredicate([&](const auto& E){return E.Id==Candidate;});TestTrue(TEXT("Name survives save/load rather than being rerolled"),Saved&&Saved->DisplayName==Expected);M.MigrateEmployeeNames();Hired=M.State.UnassignedEmployees.FindByPredicate([&](const auto& E){return E.Id==Candidate;});TestTrue(TEXT("Current staff are not renamed by migration"),Hired&&Hired->DisplayName==Expected);return true;
}
#endif
